{ stdenv, lib }:

stdenv.mkDerivation rec {
  pname = "superfetch";
  version = "1.0.0";

  src = ./.;

  # Pass PREFIX and BASHCOMPDIR to the Makefile to ensure everything installs
  # into the Nix store instead of trying to write to /usr or /etc.
  makeFlags = [ 
    "PREFIX=${placeholder "out"}"
    "BASHCOMPDIR=${placeholder "out"}/share/bash-completion/completions" 
  ];

  meta = with lib; {
    description = "A blazing fast, sub-millisecond system information tool for Linux";
    homepage = "https://github.com/TouchOfDeath/superfetch";
    license = licenses.mit;
    platforms = platforms.linux;
    maintainers = [ ];
  };
}
