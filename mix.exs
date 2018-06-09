defmodule GpioRpi.Mixfile do
  use Mix.Project

  def project do
    [app: :gpio_rpi,
      version: "0.2.2",
      elixir: "~> 1.2",
      name: "gpio_rpi",
      description: description(),
      package: package(),
      source_url: "https://github.com/Hermanverschooten/gpio_rpi",
      compilers: [:elixir_make] ++ Mix.compilers,
      make_clean: ["clean"],
      docs: [extras: ["README.md"]],
      aliases: ["docs": ["docs", &copy_images/1]],
      build_embedded: Mix.env == :prod,
      start_permanent: Mix.env == :prod,
      deps: deps()]
  end

  # Configuration for the OTP application
  #
  # Type "mix help compile.app" for more information
  def application, do: []

  defp description do
    """
    Elixir access to the GPIO interface on Raspberry PI>
    """
  end

  defp package do
    %{files: ["lib", "src/*.[ch]", "mix.exs", "README.md", "LICENSE", "Makefile"],
      maintainers: ["Herman verschooten","Frank Hunleth"],
      licenses: ["Apache-2.0"],
      links: %{"Github" => "https://github.com/Hermanverschooten/gpio_rpi"}}
  end

  defp deps do
    [
      {:elixir_make, "~>0.3"},
      {:ex_doc, "~> 0.11", only: :dev},
    ]
  end

  # Copy the images referenced by docs, since ex_doc doesn't do this.
  defp copy_images(_) do
    File.cp_r "assets", "doc/assets"
  end
end
