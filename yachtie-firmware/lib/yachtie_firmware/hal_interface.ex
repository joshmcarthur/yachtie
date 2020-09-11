defmodule Yachtie.Firmware.HALInterface do
  @moduledoc """
    Interfaces between the firmware application, the engine
    running the Phoenix web UI, and the Arduino project running
    yachtie-hal, with bidirectional communication via serial port. 
  """
  use GenServer
  alias Circuits.UART
  alias Phoenix.PubSub
  require Logger

  @doc """
    Starts the GenServer this module represents. The port and speed can be
    configured by passing in a map with appropriate keys, but are otherwise
    assumed to be the default serial port on a Raspberry Pi 0W - /dev/ttyAMA0,
    and the baud rate to be the serial baud rate configured in
    yachtie-hal/platformio.ini
  """
  def start_link(state \\ %{}) do
    state =
      state
      |> Map.put_new(:port, "ttyAMA0")
      |> Map.put_new(:speed, 115_200)

    GenServer.start_link(__MODULE__, state, name: __MODULE__)
  end

  @doc """
    Frontend function to initialize the Genserver. This function sends a message
    to tell the process to initialize and otherwise sets up the state.
  """
  def init(state) do
    send(self(), :init)
    {:ok, state}
  end

  @doc """
    Frontend function to send a type-value command to HAL.
  """
  def command(type, value) when is_binary(type) do
    GenServer.cast(__MODULE__, {:command, %{type: type, value: value}})
  end

  @doc """
    Initialize the serial connection and GenServer. This is the backend function
    to the front-end init/1 call. The port and speed may be configured by
    passing into start_link/1, however messages are otherwie assumed to be
    line-framed and separated by a newline character '\n'. The PID of the UART
    process is added to the state of this GenServer.
  """
  def handle_info(:init, %{port: port, speed: speed} = state) do
    {:ok, uart} = UART.start_link()

    :ok =
      UART.open(uart, port, speed: speed, framing: {Circuits.UART.Framing.Line, separator: "\n"})

    {:noreply, state |> Map.put(:uart, uart)}
  end

  @doc """
    Receive and parse a message from HAL. The message is required to be valid
    JSON. If the message fails to decode, an error is logged, however the
    GenServer continues to process messages. This is an important distinction
    since there are already examples where some message types can emit malformed
    or unknown characters. This allows us to log and ignore these messages.
  """
  def handle_info({:circuits_uart, name, message}, %{port: port} = state) when name == port do
    case Jason.decode(message, keys: :atoms) do
      {:ok, decoded} ->
        send(self(), {:yachtie_hal, decoded})

      {:error, error} ->
        Logger.error("#{Jason.DecodeError.message(error)}: #{message}")
    end

    {:noreply, state}
  end

  @doc """
    Log any message that has been parsed from HAL successfully, but is not
    otherwise handled by another function. These messages are logged rather than
    broadcast since, without seeing the shape of the data, we can't see what the
    effect could or should be.
  """
  # def handle_info({:yachtie_hal, decoded}, state) do
  #   inspect(decoded) |> Logger.info()
  #   {:noreply, state}
  # end

  @doc """
    For any simple key-value measurement, broadcast the type and value as a
    channel topic, with the message being the value. 
  """
  def handle_info({:yachtie_hal, %{type: type, value: value}}, state) do
    PubSub.broadcast(Yachtie.PubSub, "measurement", {String.to_atom(type), value})

    {:noreply, state}
  end


  @doc """
    Handle unknown messages with a no-op. This prevents the GenServer from
    crashing when unknown messages are returned.
  """
  def handle_info(_msg, state) do
    {:noreply, state}
  end

  @doc """
    Accept a command key-value (type & value), and send across the UART port to
    the HAL hardware. The hardware MAY respond with a reply asynchrously. This
    function will be unaware of that reply, however.
  """
  def handle_cast({:command, %{type: key, value: value}}, %{uart: uart} = state) do
    {:ok, raw_message} = Jason.encode(%{type: key, value: value})
    :ok = UART.write(uart, raw_message)
    {:noreply, state}
  end
end
