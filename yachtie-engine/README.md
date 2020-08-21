# Yachtie Engine

This mix project encompasses the engine and web UI for Yachtie.
The engine is intended to broker communications between various hardware devices (at this stage, a Raspberry Pi 0 that also runs the engine and web UI (see [firmware]), and an Arduino for interfacing with low-level sensors (see [HAL]).

The project uses LiveView to present UI. This allows backend state to change and have the UI automatically updated. LiveView also takes care of some nice connection status indication and reconnects.

State is managed with a shaky structure of GenServers and Agents. At this stage, there is no persisted data. At some point in the future, there may be, however this also needs to be supported within the firmware.

To start the server:

  * Install dependencies with `mix deps.get`
  * Install Node.js dependencies with `npm install --prefix=assets`
  * Start Phoenix endpoint with `mix phx.server`

Now you can visit [`localhost:4000`](http://localhost:4000) from your browser.

To prepare a firmware release:

  * Build the static assets with `npm --prefix=assets run deploy`

## Phoenix resources

  * Official website: https://www.phoenixframework.org/
  * Guides: https://hexdocs.pm/phoenix/overview.html
  * Docs: https://hexdocs.pm/phoenix
  * Forum: https://elixirforum.com/c/phoenix-forum
  * Source: https://github.com/phoenixframework/phoenix
