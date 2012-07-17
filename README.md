AudioPipe
=========

Play the sound using un*x pipe(for Mac OS X)

usage
==
* play white noise
cat /dev/random | audiopipe -r 44100

* one liner synthesizer: http://countercomplex.blogspot.jp/2011/10/algorithmic-symphonies-from-one-line-of.html
perl -e 'for(;;$t++){print pack("C",$t*((($t>>12)|($t>>8))&(63&($t>>4))));}' | audiopipe

