defmodule YachtieWeb.ManualControlLive do
  use YachtieWeb, :live_view
  @default_rudder 0

  @impl true
  def mount(_params, _session, socket) do
    if connected?(socket) do
      YachtieWeb.Endpoint.subscribe(Yachtie.Rudder.State.topic())
      YachtieWeb.Endpoint.subscribe(Yachtie.Location.State.topic())
    end

    {:ok,
     assign(socket,
       rudder: Yachtie.Rudder.State.get() || @default_rudder,
       location: Yachtie.Location.State.get()
     )}
  end

  @impl true
  def handle_event("control-input", %{"rudder" => %{"degrees" => degrees}}, socket) do
    {int_degrees, _remain} = Integer.parse(degrees)
    Yachtie.Rudder.State.set(int_degrees)
    {:noreply, socket}
  end

  @impl true
  def handle_info({:rudder, degrees}, socket), do: {:noreply, assign(socket, rudder: degrees)}

  @impl true
  def handle_info({:location, location}, socket),
    do: {:noreply, assign(socket, location: location)}
end
