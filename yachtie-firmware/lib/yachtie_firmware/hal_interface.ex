defmodule Yachtie.Firmware.HALInterface do
  use GenServer
  alias Circuits.UART
  alias Yachtie.PubSub
  require Logger

  def start_link(state \\ %{}) do
    state = state |> Map.put_new(:port, "ttyAMA0")
                  |> Map.put_new(:speed, 115200)
    GenServer.start_link(__MODULE__, state, name: __MODULE__)
  end

  def init(state) do
    send self(), :init
    {:ok, state}
  end

  def handle_info(:init, %{port: port, speed: speed} = state) do
    {:ok, uart} = UART.start_link()
    :ok = UART.open(uart, port, speed: speed, framing: {Circuits.UART.Framing.Line, separator: "\n"})
    {:noreply, state |> Map.put(:uart, uart)}
  end

  def handle_info({:circuits_uart, name, message}, %{port: port} = state) when name == port do
    case Jason.decode(message, keys: :atoms) do
      {:ok, decoded} ->
        send(self(), {:yachtie_hal, decoded})
      {:error, error} -> Logger.error(Jason.DecodeError.message(error))
    end

    {:noreply, state}
  end

  def handle_info({:yachtie_hal, decoded}, state) when is_map(decoded) do
    inspect(decoded) |> Logger.info()
    {:noreply, state}
  end
  
  def handle_info(_msg, state) do
    {:noreply, state}
  end

  def handle_cast({:command, %{type: key, value: value}}, %{uart: uart} = state) do
    {:ok, raw_message} = Jason.encode(%{type: key, value: value})
    :ok = UART.write(uart, raw_message)
    {:noreply, state}
  end
end
