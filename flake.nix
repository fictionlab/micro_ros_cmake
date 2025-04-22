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

        # Load a uv workspace from a workspace root.
        # Uv2nix treats all uv projects as workspace projects.
        workspace = uv2nix.lib.workspace.loadWorkspace { workspaceRoot = ./.; };

        # Create package overlay from workspace.
        overlay = workspace.mkPyprojectOverlay {
          # Prefer prebuilt binary wheels as a package source.
          # Sdists are less likely to "just work" because of the metadata missing from uv.lock.
          # Binary wheels are more likely to, but may still require overrides for library dependencies.
          sourcePreference = "wheel"; # or sourcePreference = "sdist";
          # Optionally customise PEP 508 environment
          # environ = {
          #   platform_release = "5.10.65";
          # };
        };

        # Extend generated overlay with build fixups
        #
        # Uv2nix can only work with what it has, and uv.lock is missing essential metadata to perform some builds.
        # This is an additional overlay implementing build fixups.
        # See:
        # - https://pyproject-nix.github.io/uv2nix/FAQ.html
        pyprojectOverrides = _final: _prev: {
          # Implement build fixups here.
          # Note that uv2nix is _not_ using Nixpkgs buildPythonPackage.
          # It's using https://pyproject-nix.github.io/pyproject.nix/build.html
          "empy" = _prev."empy".overrideAttrs (oldAttrs: {
            nativeBuildInputs = oldAttrs.nativeBuildInputs or [ ]
              ++ _final.resolveBuildSystem { setuptools = [ ]; };
          });
        };

        # Use Python 3.12 from nixpkgs
        python = pkgs.python312;

        # Construct package set
        pythonSet =
          # Use base package set from pyproject.nix builders
          (pkgs.callPackage pyproject-nix.build.packages {
            inherit python;
          }).overrideScope (nixpkgs.lib.composeManyExtensions [
            pyproject-build-systems.overlays.default
            overlay
            pyprojectOverrides
          ]);

        # Create an overlay enabling editable mode for all local dependencies.
        editableOverlay = workspace.mkEditablePyprojectOverlay {
          # Use environment variable
          root = "$REPO_ROOT";
          # Optional: Only enable editable for these packages
          # members = [ "hello-world" ];
        };

        # Override previous set with our overrideable overlay.
        editablePythonSet = pythonSet.overrideScope
          (nixpkgs.lib.composeManyExtensions [ editableOverlay ]);

        virtualenv =
          editablePythonSet.mkVirtualEnv "micro-ros-dev-env" workspace.deps.all;

      in {
        devShells.default = pkgs.mkShellNoCC { packages = [ virtualenv ]; };
        formatter = pkgs.nixfmt-classic;
      });
}
