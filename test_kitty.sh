#!/bin/bash
wget -qO sample.png https://www.w3.org/Icons/w3c_home
B64=$(base64 -w0 sample.png)

echo -ne "\n\n\n\n\n"
echo -ne "\033[5A"

# Print image (r=5)
echo -ne "\033_Ga=T,f=100,r=5,c=10;${B64}\033\\"
echo -ne "\033[5A"
echo -ne "\033[12C Line 1\n"
echo -ne "\033[12C Line 2\n"
echo -ne "\033[12C Line 3\n"
echo -ne "\033[12C Line 4\n"
echo -ne "\033[12C Line 5\n"
