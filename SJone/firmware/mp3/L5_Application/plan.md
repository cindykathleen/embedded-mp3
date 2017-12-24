------------------

open
close
restart
get file size
read segment
is file open
get file name
get header info
get artist
get title
get genre
rewind
set direction
get direction
get percentage

-----------------

init
print
next
prev
get size
shuffle
get4
get track list
convert to short name
get short name
get headers
set current track
get current track

------------------

each song contains:
32 bytes long name
<!-- 32 bytes short name -->
32 bytes artist
32 bytes title
32 bytes genre
160 bytes total
20 songs = 2120 or roughly 2KB
------------------

1. Read SD card and parse mp3 files
2. Store array of file information
3. Parse ID3 tags
