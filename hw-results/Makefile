INCLUDES =
EXCLUDES =

comma = ,
spcae = " "
include_args = $(subst $(space),$(comma),$(INCLUDES))
exclude_args = $(subst $(space),$(comma),$(EXCLUDES))
TABULAR_ARGS = -d rpi4b/ -d qemu/ $(if $(INCLUDES),--includes=$(include_args)) $(if $(EXCLUDES),--excludes=$(exclude_args))

.PHONY: all
all:  fetchrpi tabular top.pdf install clean

.PHONY: fetchrpi
fetchrpi:
	scp 'rpi4:overnights/*.log' rpi4b/

.PHONY: tabular
tabular:
	python3 tabular.py $(TABULAR_ARGS)
	python3 tabular.py $(TABULAR_ARGS) --standalone

.PHONY: top.pdf
top.pdf:
	pdflatex top.tex
	pdflatex top.tex

.PHONY: install
install:
	cp top.pdf built_doc/results.pdf

.PHONY: clean
clean:
	rm -f top.tex
	rm -f top.aux
	rm -f top.log
	rm -f top.pdf