defmodule ElixirAleRpi.Mixfile do
  use Mix.Project

  def project do
    [app: :elixir_ale_rpi,
      version: "0.1.0",
      build_path: "../../_build",
      config_path: "../../config/config.exs",
      deps_path: "../../deps",
      lockfile: "../../mix.lock",
      elixir: "~> 1.3",
      name: "elixir_ale_rpi",
      description: description(),
      package: package(),
      source_url: "https://github.com/Hermanverschooten/elixir_ale_rpi",
      compilers: [:elixir_make] ++ Mix.compilers,
      make_clean: ["clean"],
      docs: [extras: ["README.md"]],
      aliases: ["docs": ["docs", &copy_images/1]],
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
    Elixir access to hardware I/O interfaces such as GPIO, I2C, and SPI.
    """
  end

  defp package do
    %{files: ["lib", "src/*.[ch]", "mix.exs", "README.md", "LICENSE", "Makefile"],
      maintainers: ["Herman verschooten","Frank Hunleth"],
      license: ["Apache-2.0"],
      links: %{"Github" => "https://github.com/Hermanverschooten/elixir_ale_rpi"}}
  end

  defp deps do
    [
      {:elixir_make, "~>0.3"},
      {:earmark, "~> 0.1", only: :dev},
      {:ex_doc, "~> 0.11", only: :dev},
      {:credo, "~> 0.3", only: [:dev, :test]}
    ]
  end

  # Copy the images referenced by docs, since ex_doc doesn't do this.
  defp copy_images(_) do
    File.cp_r "assets", "doc/assets"
  end
end
