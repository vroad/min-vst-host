{ lib
, stdenv
, fetchFromGitHub
, cmake
, pkg-config
, xorg
, jack2
}:

let
  vst3sdk = fetchFromGitHub {
    owner = "steinbergmedia";
    repo = "vst3sdk";
    rev = "43b4e366ff84afc9b4247776acbf5e234683b77f";
    sha256 = "sha256-Tyh8InZhriRh2bP9YqaXoVv33CYlwro9mpBKmoPNTfU=";
    fetchSubmodules = true;
    deepClone = false;
  };

  cleanSrc = lib.cleanSourceWith {
    src = ./.;
    filter = path: type:
      lib.hasInfix "/libs/" path == false;
  };

in
stdenv.mkDerivation {
  pname = "min-vst-host";
  version = "0.1";
  src = cleanSrc;

  nativeBuildInputs = [ cmake pkg-config ];
  buildInputs = [ xorg.libX11 jack2 ];

  preConfigure = ''
    mkdir -p libs
    ln -s ${vst3sdk} libs/vst3sdk
  '';

  installPhase = ''
    mkdir -p $out/bin
    cp bin/Release/min-vst-host $out/bin/
  '';
}
