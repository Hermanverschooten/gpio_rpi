defmodule AleRpi.Mixfile do
  use Mix.Project

  def project do
    [app: :ale_rpi,
     version: "0.1.0",
     build_path: "../../_build",
     config_path: "../../config/config.exs",
     deps_path: "../../deps",
     lockfile: "../../mix.lock",
     elixir: "~> 1.2",
     name: "ale_rpi",
     description: description(),
     package: package(),
     source_url: "https://github.com/Hermanverschooten/ale_rpi",
     compilers: [:elixir_make] ++ Mix.compilers,
     make_clean: ["clean"],
     build_embedded: Mix.env == :prod,
     start_permanent: Mix.env == :prod,
     deps: deps]
  end

  # Configuration for the OTP application
  #
  # Type "mix help compile.app" for more information
  def application, do: []

  defp description do
    """
    Elixir access to PULL-UP registers for RPI GPIO.
    """
  end

  defp package do
    %{files: ["lib", "src/*.[ch]", "mix.exs", "README.md", "LICENSE", "Makefile"],
      maintainers: ["Herman verschooten"],
      license: ["Apache-2.0"],
      links: %{"Github" => "https://github.com/Hermanverschooten/ale_rpi"}}
  end

  defp deps do
    [
      {:elixir_make, "~>0.3"}
    ]
  end
end
