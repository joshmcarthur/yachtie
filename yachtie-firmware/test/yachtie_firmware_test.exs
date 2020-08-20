defmodule YachtieFirmwareTest do
  use ExUnit.Case
  doctest YachtieFirmware

  test "greets the world" do
    assert YachtieFirmware.hello() == :world
  end

  test "configures the expected JSON library" do
    assert Application.get_env(:yachtie, :json_library) == Jason
  end

  test "configures the web UI in the expected way" do
    web_config = Application.get_env(:yachtie, YachtieWeb.Endpoint)
    assert web_config[:url][:host], "yachtie.local"
    assert String.length(web_config[:secret_key_base]) >= 64
  end

  test "configures the web UI to start a server" do
    web_config = Application.get_env(:yachtie, YachtieWeb.Endpoint)
    assert web_config[:server]
  end
end
