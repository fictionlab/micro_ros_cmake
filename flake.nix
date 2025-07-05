{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    pyproject-nix = {
      url = "github:pyproject-nix/pyproject.nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    uv2nix = {
      url = "github:pyproject-nix/uv2nix";
      inputs.pyproject-nix.follows = "pyproject-nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    pyproject-build-systems = {
      url = "github:pyproject-nix/build-system-pkgs";
      inputs.pyproject-nix.follows = "pyproject-nix";
      inputs.uv2nix.follows = "uv2nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };
  outputs = { nixpkgs, flake-utils, pyproject-nix, uv2nix
    , pyproject-build-systems, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = (import nixpkgs { system = system; });

        workspace = uv2nix.lib.workspace.loadWorkspace { workspaceRoot = ./.; };

        overlay = workspace.mkPyprojectOverlay { sourcePreference = "wheel"; };

        pyprojectOverrides = _final: _prev: {
          "empy" = _prev."empy".overrideAttrs (oldAttrs: {
            nativeBuildInputs = oldAttrs.nativeBuildInputs or [ ]
              ++ _final.resolveBuildSystem { setuptools = [ ]; };
          });
        };

        pythonSet = (pkgs.callPackage pyproject-nix.build.packages {
          python = pkgs.python312;
        }).overrideScope (nixpkgs.lib.composeManyExtensions [
          pyproject-build-systems.overlays.default
          overlay
          pyprojectOverrides
        ]);

        pythonVirtEnv =
          pythonSet.mkVirtualEnv "micro-ros-dev-env" workspace.deps.all;

      in {
        devShells.default = pkgs.mkShellNoCC { packages = [ pythonVirtEnv ]; };
        formatter = pkgs.nixfmt-classic;
      });
}
