PDF = userguide.pdf
IMAGES = $(wildcard images/*.pdf)

$(PDF): userguide.tex $(IMAGES)
	pdfcsplain $<
