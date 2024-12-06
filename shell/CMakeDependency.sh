clear

pushd ./build

cmake .. --graphviz=graph.dot && dot -Tpng graph.dot -o graphImage.png

popd
