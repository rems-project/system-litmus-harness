INCLUDES =
EXCLUDES =

comma = ,
spcae = " "
include_args = $(subst $(space),$(comma),$(INCLUDES))
exclude_args = $(subst $(space),$(comma),$(EXCLUDES))
TABULAR_ARGS = -d rpi4b/ -d qemu/ -d rpi3bp/ -d graviton2/ $(if $(INCLUDES),--includes=$(include_args)) $(if $(EXCLUDES),--excludes=$(exclude_args))

.PHONY: all
all: tabular top.pdf install clean

.PHONY: fetch
fetch:
	scp 'rpi4:overnights/*.log' rpi4b/
	scp 'rpi3:overnights/*.log' rpi3bp/
	scp 'aws:*.log' graviton2/
	python3 filter_erratas.py || echo failed to filter errata

.PHONY: tabular
tabular:
	python3 tabular.py $(TABULAR_ARGS) --standalone

top.pdf: top.tex
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
