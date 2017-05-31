.PHONY = all clean

IMAGES = $(wildcard images/*.pdf)

all: userguide.pdf internals.pdf

userguide.pdf: userguide.tex $(IMAGES)
	pdftex $<

internals.pdf: internals.tex
	pdftex $<

clean:
	rm *.pdf
