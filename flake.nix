{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    poetry2nix = {
      url = "github:nix-community/poetry2nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };
  outputs = { self, nixpkgs, flake-utils, poetry2nix }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = (import nixpkgs { system = system; });

        inherit (poetry2nix.lib.mkPoetry2Nix { inherit pkgs; }) mkPoetryEnv;

        poetry-env = mkPoetryEnv {
          projectDir = ./.;
          python = pkgs.python312;
        };

      in {
        devShells.default = pkgs.mkShellNoCC {
          # PYTHONPATH = "${poetry-env}/lib/python3.12/site-packages";
          packages = [ poetry-env ];
        };
        formatter = pkgs.nixfmt-classic;
      });
}
