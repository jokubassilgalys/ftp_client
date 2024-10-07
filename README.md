This repository contains a custom FTP client written in C made for unix operating systems.
It is able to connect to any FTP server and send or recieve files of any type.
In addition to commonly supported FTP commands this client also has a few custom commands such as:
- <b>mput</b> [filepath]/*.[extension]
- <b>mget</b> [filepath]/*.[extension]
- <b>mzput</b> [filepath]/*.[extension]
- <b>mzget</b> [filepath]/*.[extension]

<b>mput</b> recursively searches the local folder indicated by [filepath] and its subfolders to find all files with the same [extension].
These files are then sent to the FTP server the client is connected to. </p>
<b>mget</b> recursively searches the FTP server's folder indicated by [filepath] and its subfolders to find all files with the same [extension].
These files are then downloaded from the server to the client's computer. </p>
<b>mzput</b> does the same thing as mput except it packs all the files in a .zip archive and sents it instead. </p>
<b>mzget</b> does the same thing as mget but the downloaded files are in a .zip archive.

