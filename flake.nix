{
  description = "";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = { self, ... }@inputs:
    let
      pkgs = inputs.nixpkgs.legacyPackages.x86_64-linux;
      buildInputs = with pkgs; [
        gcc
        gnumake
        pkg-config

        glfw
        glm
        stb
        assimp
      ];
    in
    {
      packages.x86_64-linux.default =
        pkgs.stdenv.mkDerivation {
          name = "ass3";
          src = self;
          buildInputs = buildInputs;
        };
      devShells.x86_64-linux.default = pkgs.mkShell {
        packages = buildInputs ++ (with pkgs; [
          gdb

          glslang
          shaderc
        ]);

        inputsFrom = with pkgs; [ ];
      };
    };
}
