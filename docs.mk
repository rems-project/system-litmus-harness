.PHONY: doc-publish
doc-publish:
	$(MAKE) -C doc publish

.PHONY: doc-hw-results
doc-hw-results:
	$(MAKE) -C hw-results EXCLUDES=checks,errata