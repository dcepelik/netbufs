#include <stdlib.h>


int main(int argc, char *argv[])
{
	char *inputfn;
	char *outputfn;
	struct cbor_stream *in;
	struct cbor_stream *out;
	struct cbor_decoder *dec;
	struct cbor_encoder *enc;
	cbor_err_t err;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <input-file> <output-file>\n", argv[0]);
		return EXIT_FAILURE;
	}

	inputfn = argv[1];
	outputfn = argv[2];

	if ((err = cbor_stream_open_file(in, inputfn, O_RDONLY)) != CBOR_ERR_OK) {
		return EXIT_FAILURE;
	}

	dec = cbor_decoder_new(in);
	if (!dec) {
		return EXIT_FAILURE;
	}

	if ((err = cbor_stream_open_file(out, outputfn, O_WRONLY)) != CBOR_ERR_OK) {
		return EXIT_FAILURE;
	}

	enc = cbor_encoder_new(out);
	if (!enc) {
		return EXIT_FAILURE;
	}

	cbor_stream_close(in);
	cbor_stream_close(out);

	cbor_decoder_delete(dec);
	cbor_encoder_delete(enc);

	return EXIT_SUCCESS;
}
