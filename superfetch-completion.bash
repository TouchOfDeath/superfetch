#!/usr/bin/env bash

_superfetch_completions()
{
    local cur="${COMP_WORDS[COMP_CWORD]}"
    COMPREPLY=( $(compgen -W "--json --version -V --help -h" -- "$cur") )
}

complete -F _superfetch_completions superfetch
