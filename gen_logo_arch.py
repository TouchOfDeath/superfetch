logo = """                   -`
                  .o+`
                 `ooo/
                `+oooo:
               `+oooooo:
               -+oooooo+:
             `/:-:++oooo+:
            `/++++/+++++++:
           `/++++++++++++++:
          `/+++ooooooooooooo/`
         ./ooosssso++osssssso+`
        .oossssso-````/ossssss+`
       -osssssso.      :ssssssso.
      :osssssss/        osssso+++.
     /ossssssss/        +ssssooo/-
   `/ossssso+/:-        -:/+osssso+-
  `+sso+:-`                 `.-/+oso:
 `++:.                           `-/+/
 .`                                 `/"""

c1 = "\\033[38;5;6m"  # Arch cyan
c2 = "\\033[38;5;6m"

print("static const char *logo_arch[] = {")
for line in logo.split('\n'):
    s = line.replace('${c1}', c1).replace('${c2}', c2).replace('\\', '\\\\')
    print(f'    "{c1}{s}",')
print("};")
