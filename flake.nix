{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-25.11";
  };

  outputs =
    { self, nixpkgs }:
    let
      pkgs = import nixpkgs { system = "x86_64-linux"; };
    in
    {
      packages.x86_64-linux = rec {
        xex = pkgs.requireFile {
          name = "default.xex";
          url = "the meats";
          hash = "sha256-CbsFg4BjQVNZO2xX/Y6Krs1iUzY6yKN1IGghK9RkLQ8=";
        };
        rexglue-sdk = pkgs.llvmPackages_20.stdenv.mkDerivation {
          pname = "rexglue-sdk";
          version = "0.2.1";
          src = pkgs.fetchFromGitHub {
            owner = "rexglue";
            repo = "rexglue-sdk";
            tag = "v0.2.1";
            fetchSubmodules = true;
            hash = "sha256-4oJ3hi4s2ih2F6cjAGdTA1mMr68EjAdBbV1mIg2qRLA=";
          };

          patches = [
            ./rexglue-nix.patch
          ];

          nativeBuildInputs = with pkgs; [
            pkg-config
            cmake
            ninja
            python3

            # audio libraries
            libpulseaudio
            pipewire
            alsa-lib
          ];

          buildInputs = with pkgs; [
            gtk3
          ];

          configurePhase = ''
            cmake --preset=linux-amd64 -DCMAKE_INSTALL_PREFIX=$out
          '';

          buildPhase = ''
            ninja -C out/build/linux-amd64/ -f build-RelWithDebInfo.ninja
          '';

          installPhase = ''
            ninja -C out/build/linux-amd64/ -f build-RelWithDebInfo.ninja install
          '';

          dontStrip = true;
        };

        band3 = pkgs.llvmPackages_20.stdenv.mkDerivation {
          pname = "band3_recomp";
          version = "git";
          src = ./.;

          nativeBuildInputs = with pkgs; [
            pkg-config
            cmake
            ninja
            rexglue-sdk
          ];

          buildInputs = with pkgs; [
            gtk3
          ];

          configurePhase = ''
            mkdir assets
            cp -a ${xex} assets/default.xex
            rexglue codegen band3_config.toml
            cmake --preset=linux-amd64-release \
              -DCMAKE_INSTALL_PREFIX=$out \
              -DCMAKE_C_COMPILER=clang \
              -DCMAKE_CXX_COMPILER=clang++
          '';

          buildPhase = ''
            ninja -C out/build/linux-amd64-release/
          '';

          installPhase = ''
            mkdir -p $out/bin
            cp out/build/linux-amd64-release/band3 $out/bin
          '';

          dontStrip = true;

          shellHook = ''
            export LD_LIBRARY_PATH="${
              pkgs.lib.makeLibraryPath [
                pkgs.wayland
                pkgs.wayland-scanner
                pkgs.libxkbcommon
                pkgs.vulkan-loader
                pkgs.libpulseaudio
                pkgs.pipewire
                pkgs.alsa-lib
                pkgs.xorg.libX11
                pkgs.xorg.libXcursor
                pkgs.xorg.libXrandr
                pkgs.xorg.libXi
              ]
            }"
          '';
        };
      };
    };
}
