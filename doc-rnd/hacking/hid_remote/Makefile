proto_parse.svg: proto_parse.dot fixlab.sh
	./fixlab.sh < proto_parse.dot > .proto_parse.dot
	dot -Tsvg .proto_parse.dot > proto_parse.svg


proto_parse.png: proto_parse.dot fixlab.sh
	./fixlab.sh < proto_parse.dot > .proto_parse.dot
	dot -Tpng .proto_parse.dot > proto_parse.png

