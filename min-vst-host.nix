{ lib
, stdenv
, fetchFromGitHub
, cmake
, pkg-config
, xorg
, jack2
, nix-gitignore
}:
let
  vst3sdkArgs = builtins.fromJSON (builtins.readFile ./vst3sdk-source.json);
  vst3sdk = fetchFromGitHub {
    inherit (vst3sdkArgs) owner repo rev sha256;
    fetchSubmodules = true;
    deepClone = false;
  };
in
stdenv.mkDerivation {
  pname = "min-vst-host";
  version = "0.1";
  src = nix-gitignore.gitignoreSource [ ] ./.;

  nativeBuildInputs = [ cmake pkg-config ];
  buildInputs = [ xorg.libX11 jack2 ];

  preConfigure = ''
    mkdir -p external
    ln -s ${vst3sdk} external/vst3sdk
  '';

  installPhase = ''
    mkdir -p $out/bin
    cp bin/Release/min-vst-host $out/bin/
  '';
}
