# AleRpi

This module allows you to change the pullup registers on a Raspberry Pi.

```elixir
AleRpi.Gpio.pullup_mode(pin, :up)
AleRpi.Gpio.pullup_mode(pin, :down)
AleRpi.Gpio.pullup_mode(pin, :off)
```

The pins are numbered according to the BCM numbering!

## Installation

If [available in Hex](https://hex.pm/docs/publish), the package can be installed as:

  1. Add `ale_rpi` to your list of dependencies in `mix.exs`:

    ```elixir
    def deps do
      [{:ale_rpi, "~> 0.1.0"}]
    end
    ```

  2. Ensure `ale_rpi` is started before your application:

    ```elixir
    def application do
      [applications: [:ale_rpi]]
    end
    ```

