defmodule YachtieWeb.ManualControlLive do
  use YachtieWeb, :live_view
  @default_rudder 0

  def mount(_params, _session, socket) do
    if connected?(socket) do
      YachtieWeb.Endpoint.subscribe(Yachtie.Rudder.State.topic())
      YachtieWeb.Endpoint.subscribe("measurement")
    end

    {:ok,
     assign(socket,
       rudder: Yachtie.Rudder.State.get() || @default_rudder,
       location: [],
       temperature: nil,
       humidity: nil,
       last_nmea: nil,
       satellites: 0,
       speed_knots: nil,
       course_degrees: nil,
       fix_age: nil
     )}
  end

  def handle_event("control-input", %{"rudder" => %{"degrees" => degrees}}, socket) do
    {int_degrees, _remain} = Integer.parse(degrees)
    Yachtie.Rudder.State.set(int_degrees)
    {:noreply, socket}
  end

  def handle_info({:temperature_celsius, temp}, socket),
    do: {:noreply, assign(socket, temperature: temp)}

  def handle_info({:humidity_rh, humidity}, socket),
    do: {:noreply, assign(socket, humidity: humidity)}

  def handle_info({:rudder, degrees}, socket), do: {:noreply, assign(socket, rudder: degrees)}
  def handle_info({:gps_nmea, nmea}, socket), do: {:noreply, assign(socket, last_nmea: nmea)}

  def handle_info({:gps_satellites, sats}, socket),
    do: {:noreply, assign(socket, satellites: sats)}

  def handle_info({:gps_speed, speed_knots}, socket),
    do: {:noreply, assign(socket, speed_knots: speed_knots)}

  def handle_info({:gps_course, course_deg}, socket),
    do: {:noreply, assign(socket, course_degrees: course_deg)}

  def handle_info({:gps_location, latlng}, socket),
    do: {:noreply, assign(socket, location: String.split(latlng, ","))}

  def handle_info({:gps_location_age, age}, socket), do: {:noreply, assign(socket, fix_age: age)}
  def handle_info(_unhandled, socket), do: {:noreply, socket}
end
