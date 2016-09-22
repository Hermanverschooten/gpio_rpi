defmodule AleRpi.Gpio do
  @doc """
  Switch the pins pullup to the given mode.
  """

  @spec pullup_mode(integer, :up | :down | :off) :: :ok | {:error, term}
  def pullup_mode(pin, :up) when is_integer(pin) do
    set_pullup(pin, "up")
  end
  def pullup_mode(pin, :down) when is_integer(pin) do
    set_pullup(pin, "down")
  end
  def pullup_mode(pin, :off) when is_integer(pin) do
    set_pullup(pin, "off")
  end
  def pullup_mode(_pin, _), do: {:error, "Invalid pin or mode"}

  defp set_pullup(pin, mode) do
    executable = :code.priv_dir(:ale_rpi) ++ '/ale_rpi'
    System.cmd executable, [pin, mode]
    :ok
  end
end
