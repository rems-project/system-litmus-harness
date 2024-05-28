.PHONY: cleantools

TOOLS_DIR = tools/

TOML_TRANSLATOR_ROOT = ./tools/litmus-toml-translator
TOML_TRANSLATOR = $(TOML_TRANSLATOR_ROOT)/target/release/litmus-toml-translator

$(TOML_TRANSLATOR):
	$(call run_cmd,CARGO,tools/litmus-toml-translator,cd tools/litmus-toml-translator && cargo build --profile=release)
TOOLS += $(TOML_TRANSLATOR)
.PHONY: $(TOML_TRANSLATOR)

cleantools:
	$(call run_cmd,CLEAN,tools/litmus-toml-translator,cd tools/litmus-toml-translator && cargo clean)

tools: $(TOOLS)
