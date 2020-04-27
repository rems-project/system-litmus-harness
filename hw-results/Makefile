.PHONY: all
all:  fetchrpi tabular top.pdf install clean

.PHONY: fetchrpi
fetchrpi:
	scp 'rpi:overnights/2020-overnight-*.log' rpi4b/

.PHONY: tabular
tabular:
	python3 tabular.py -d rpi4b/ -d qemu/ --standalone

.PHONY: top.pdf
top.pdf:
	pdflatex top.tex
	pdflatex top.tex

.PHONY: install
install:
	cp top.pdf built_doc/results.pdf

.PHONY: clean
clean:
	rm -f results-*.tex
	rm -f top.tex
	rm -f top.aux
	rm -f top.log
	rm -f top.pdf