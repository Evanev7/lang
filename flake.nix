{
  description = "RUST FLAKE RUST FLAKE";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    {
      self,
      nixpkgs,
    }:
    let
      system = "x86_64-linux";
      overlays = [ ];
      pkgs = import nixpkgs {
        inherit system overlays;
      };
    in
    with pkgs;
    {
      devShells.${system}.default = mkShell rec {
        buildInputs = [
          cargo
          rustc
          rust-analyzer
          rustfmt
          bacon
          clippy
          gcc
          python3
        ];

        RUST_SRC_PATH = "${pkgs.rust.packages.stable.rustPlatform.rustLibSrc}";
        LD_LIBRARY_PATH = "$LD_LIBRARY_PATH:${builtins.toString (pkgs.lib.makeLibraryPath buildInputs)}";
      };
    };
}
