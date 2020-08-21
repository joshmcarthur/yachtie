defmodule Yachtie.Location.State do
  use Agent
  alias Phoenix.PubSub

  @pubsub_application Yachtie.PubSub
  @pubsub_topic "location"

  def start_link(_args) do
    Agent.start_link(fn -> nil end, name: __MODULE__)
  end

  def get() do
    Agent.get(__MODULE__, fn location -> location end)
  end

  def topic do
    @pubsub_topic
  end

  def set(location = %{latitude: _latitude, longitude: _longitude}) do
    PubSub.broadcast(@pubsub_application, topic(), {:location, location})
    Agent.update(__MODULE__, fn old_location -> location end)
  end
end
