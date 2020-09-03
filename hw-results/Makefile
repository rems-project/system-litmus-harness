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
	mkdir -p raw/qemu/
	mkdir -p raw/rpi4b/
	mkdir -p raw/rpi3bp/
	mkdir -p raw/graviton2/

	scp 'rpi4:overnights/*.log' raw/rpi4b/
	scp 'rpi3:overnights/*.log' raw/rpi3bp/
	scp 'aws:*.log' raw/graviton2/

	python3 filter_erratas.py || echo failed to filter errata

	tar -vczf raw/qemu.tar.gz raw/qemu/
	tar -vczf raw/rpi3bp.tar.gz raw/rpi3bp/
	tar -vczf raw/rpi4b.tar.gz raw/rpi4b/
	tar -vczf raw/graviton2.tar.gz raw/graviton2/

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
