defmodule Yachtie.Rudder.State do
  use Agent
  alias Phoenix.PubSub

  @pubsub_application Yachtie.PubSub
  @pubsub_topic "rudder"
  @max_rudder 20
  @min_rudder -20
  @default_rudder 0

  def start_link(_args) do
    Agent.start_link(fn -> @default_rudder end, name: __MODULE__)
  end

  def get() do
    Agent.get(__MODULE__, fn location -> location end)
  end

  def topic do
    @pubsub_topic
  end

  def set(degrees) when @min_rudder <= degrees and degrees <= @max_rudder do
    PubSub.broadcast(@pubsub_application, topic(), {:rudder, degrees})
    Agent.update(__MODULE__, fn old_val -> degrees end)
  end
end
