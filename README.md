# ShellFS

## Overview
L'obiettivo è realizzare un file system di tipo FAT con una shell annessa.
Il FS sarà un file mmappato e persistente che potrà essere "smontato e montato"
da più PC continuando a mantenere il contenuto.
Sarà possibile interagire col FS attraverso i comandi propri della shell Linux,
quali `cat`, `ls`, `rm` ecc.

# Funzionamento
La shell, una volta runnata fornisce un'interfaccia di tre comandi per interagire 
con i file system (`format`, `open` e `close`). Una volta aperto un file system con il 
comando `open`, esso va a bloccare le operazioni `format` ed `open` per altri file.
In questa modalità vengono sbloccati i comandi di shell (`mkdir`, `ls`, ecc..) per il file system
appena aperto e sarà possibile ritornare allo stato originale solo con il comando `close`. 

## Comandi disponibili

### Comandi file system
- `format <file_system> <size>`
- `open   <file_system>`
- `close`

### Comandi shell
- `mkdir  <dir>`
- `cd     <dir | / | .. | .>`
- `touch  <file>`
- `cat    <file>`
- `ls     <dir>`
- `append <file> <text>`
- `rm     <dir/file>`

### Comandi general purpose
- `help`
- `quit`
- `clear`
