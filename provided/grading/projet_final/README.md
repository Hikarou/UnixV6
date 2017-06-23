Nous avons été jusqu'au bout du projet.
Nous estimons la charge de travail entre 8 et 10h par semaine par personne.

Concernant le style de programation, nous avons choisi de déclarer un maximum nos variables en haut d'une fonction. Par ailleurs, pour tous les tableaux dont la taille n'est pas connue au moment de la compliation, nous avons choisi de travailler avec des pointeurs et non pas avec des "variable length array" dont C99 permet l'utilisation. 

Voici quelques explications par rapport aux fonctions filev6_writebytes, filev6_writesector, write_big_file, write_small_file, write_change qui n'ont pas été conçues selon les recommandations du prof. 
Pour écrire dans un fichier, il faut appeler filev6_writebytes. Celle-ci fait le tri entre trois cas: si c'est un petit fichier et que le fichier va rester petit, elle appelle write_small_file. Si c'est un petit fichier, mais qu'il va devenir grand lors de l'écriture, elle remplit d'abord complètement le petit fichier avec write_small_file, puis elle le change avec write_change, puis elle écrit la suite avec write_big_file. Si fichier est déjà grand, write_big_file est appelée directement. Pour écrire, write_big_file, write_change, write_small_file appellent toutes filev6_writesector. 

filev6_writesector:
cette fonciton n'écrit qu'un seul sector à la fois. C'est elle qui regarde si le dernier secteur (passé en paramètre) est plein ou non. Si elle peut écrire dans ce dernier, alors, elle le remplit et renvoie le numéro de secteur 0 quand elle n'a PAS écrit dans un nouveau secteur. Si le contraire se produit, elle renvoie le numero de secteur dans lequel elle a écrit. On peut forcer la fonction à écrire dans un nouveau secteur en lui passant un "sector_number" inférieur à 1. Par ailleurs elle n'agit jamais sur les inodes, bien que celle-ci les lise afin de connaître la taille du fichier traité.

write_small_file:
Fonction très simple: tant qu'il reste des caractères à écrire, filev6_writesector est appelée. L'inode est changé après chaque appel à filev6_writesector.

write_change: 
cette foncition prend un fichier petit plein et le change en un grand fichier. Pour cela, les adresses des secteurs sont copiées. L'idée d'écrire dans un nouveau secteur ces adresses. Pour cela, la fonction filev6_writesector est appelée, mais pour éviter qu'elle écrive à la fin d'un secteur, nous "trompons la fonction" en en modifiant la taille de l'inode à 0. Ainsi, nous pouvons être sûrs qu'elle écrira dans un nouveau secteur. Ensuite, la nouvelle taille est écrite à nouveau avec une nouvelle adresse seulement. 

write_big_file:
Pour écrire dans un grand fichier, la méthodologie suivante est faite. Un "faux fichier" est créé, c'est-à-dire que ce fichier n'existe pas. Celui est un petit fichier dont son contenu est les adresses des secteurs qui contiennent les data du grand fichier. Ceci offre l'avantage que ce petit fichier (c'est-à-dire les adresses du grand fichier, le vrai) peuvent être traitée avec la fonction write_small_file. 
Au début de la fonction, on regarde si le dernier secteur de data est plein ou non, afin de le remplir si ce n'est pas le cas. Puis, on écrit dans le grand fichier, et les adresses des secteurs dans lesquelles ont écrit sont directement écrite dans le petit fichier contenant les adresses.
A la fin, on met ensemble les données des inodes du faux petit fichier et du grand fichier ensemble pour former le vrai inode: il contient les adresses du petit fichier mais la taille du grand.


