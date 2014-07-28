mkdir -p puppetlabs-icfp-2014/solution
lein run *.llcoolj
mv lambdaman.gcc puppetlabs-icfp-2014/solution/lambdaman.gcc
lein run ghost.gfk
mv ghost.ghc puppetlabs-icfp-2014/solution/ghost0.ghc
mkdir -p puppetlabs-icfp-2014/code
mv `ls | grep -v puppetlabs-icfp-2014` puppetlabs-icfp-2014/code
rm -rf puppetlabs-icfp-2014/stale puppetlabs-icfp-2014/classes
mv puppetlabs-icfp-2014/code/README.md puppetlabs-icfp-2014/code/README
tar czvf puppetlabs-icfp-2014.tar.gz puppetlabs-icfp-2014
