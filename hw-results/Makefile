INCLUDES =
EXCLUDES =

comma = ,
spcae = " "
include_args = $(subst $(space),$(comma),$(INCLUDES))
exclude_args = $(subst $(space),$(comma),$(EXCLUDES))
TABULAR_ARGS = -d rpi4b/ -d qemu/ -d rpi3bp/ -d graviton2/ $(if $(INCLUDES),--includes=$(include_args)) $(if $(EXCLUDES),--excludes=$(exclude_args))
TAB = python3 tabular.py $(TABULAR_ARGS) --standalone

PDF = pdflatex

define makepdf
   $(PDF) $1
   $(PDF) $1
endef

.PHONY: all
all: results.pdf results-exceptions.pdf install clean

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

.PHONY: results.tex
results.tex:
	$(TAB) --excludes='@errata' --standalone-file='results.tex'

.PHONY: results-exceptions.tex
results-exceptions.tex:
results-exceptions.tex:
	$(TAB) --includes='@exc' --standalone-file='results-exceptions.tex'

%.pdf: %.tex
	$(call makepdf,$^)

.PHONY: install
install:
	cp *.pdf built_doc/

define RM
	rm -f $1.tex
	rm -f $1.aux
	rm -f $1.log
	rm -f $1.pdf
endef

.PHONY: clean
clean:
	$(call RM,results)
	$(call RM,results-exceptions)