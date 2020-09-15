.PHONY: publish
publish:
	$(MAKE) -C doc publish

.PHONY: hw-results
hw-results:
	$(MAKE) -C hw-results EXCLUDES=checks,errata