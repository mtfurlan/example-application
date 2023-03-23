.PHONY: help
help:   ## Show this help.
	@awk 'BEGIN {FS = ":.*##"; printf "Usage: make \033[36m[target]\033[0m\nWe will pull dependency dirs out of env if exist like LIBSM_DIR\n"} /^[0-9a-zA-Z_-]+:.*?##/ { printf "  \033[36m%-10s\033[0m %s\n", $$1, $$2 } /^##@/ { printf "\n\033[1m%s\033[0m\n", substr($$0, 5) } ' $(MAKEFILE_LIST)

REPO_ROOT ?= ../

BOARDS := nrf5340dk_nrf5340_cpuapp rf52840dk_nrf52840
BOARD ?= nrf5340dk_nrf5340_cpuapp

ifeq ($(BOARD),nrf5340dk_nrf5340_cpuapp)
BOARD_PROCESSOR ?= nRF5340_xxAA_APP
else
BOARD_PROCESSOR ?= nRF52840_xxAA
endif

_RTTPYFLAGS = -w8 -d $(BOARD_PROCESSOR)
# if DEVICE passed in, only deal with that device
ifneq ($(DEVICE),)
_NRFJPROGFLAGS =  -s $(DEVICE)
_RTTPYFLAGS += -s $(DEVICE)
_WESTFLAGS = --dev-id=$(DEVICE)
_TWISTERWESTFLAGS = ,$(_WESTFLAGS)
_RTTFLAGS = --device $(DEVICE)
else
endif

define buildMacro =
	west build -b $(1) $(2)

endef

.DEFAULT_GOAL := app
.PHONY: app
app: ## build app, optional BOARD= var
	$(call buildMacro,$(BOARD), app)

.PHONY: app-pristine
app-pristine: ## build app pristine, optional BOARD= var
	$(call buildMacro,$(BOARD),--pristine=always app)

.PHONY: app-spi
app-spi: ## build app-spi, optional BOARD= var
	$(call buildMacro,$(BOARD), app-spi)

.PHONY: app-spi-pristine
app-spi-pristine: ## build app-spi pristine, optional BOARD= var
	$(call buildMacro,$(BOARD),--pristine=always app-spi)

.PHONY: menuconfig
menuconfig: ## menuconfig
	west build -t menuconfig

.PHONY: list-boards
list-boards: ## list supported boards
	@echo "$(subst $() $(),\\n,$(BOARDS))"

.PHONY: debug
debug: ## debug
	west debug $(_WESTFLAGS)

.PHONY: flash
flash: ## flash
	west flash --erase --softreset $(_WESTFLAGS)

.PHONY: reset
reset: ## reset the board
	nrfjprog $(_NRFJPROGFLAGS) --reset

.PHONY: rtt
rtt: ## run rtt.sh to get RTT output
	$(REPO_ROOT)/tooling/rtt.sh $(_RTTFLAGS)

.PHONY: clean
clean: ## delete build files
	rm -rf build twister-out*

TWISTER_BOARDS=$(patsubst %, -p %, $(BOARDS))
TWISTER_COMMAND=$(REPO_ROOT)/zephyr/scripts/twister -vv --disable-warnings-as-errors --clobber-output --board-root $(REPO_ROOT)/b2v/boards -T ./tests

.PHONY: twister-build-all
twister-build-all: ## build twister for all boards
	$(TWISTER_COMMAND) $(TWISTER_BOARDS)

.PHONY: twister
twister: ## Run tests on actual hardware
	$(TWISTER_COMMAND) -p $(BOARD) --device-testing \
		--device-serial-pty="../tooling/seggerRTT/SeggerRTTListener.py $(_RTTPYFLAGS)" \
		--west-runner=nrfjprog \
		--west-flash="--erase,--softreset$(_TWISTERWESTFLAGS)"

.PHONY: twister-all
twister-all: ## Run tests on actual hardware, requires hardware map (see README)
	# (can filter with BOARD=)
	$(TWISTER_COMMAND) --device-testing --hardware-map $(REPO_ROOT)/tooling/twister-map.yml
	#$(patsubst %, -p %, $(BOARD))


.PHONY: test-build
test-build: ## You can build individual tests like 'make test-ublox' and then flash them with make flash.
	@echo "Don't run make test-build itself, this is just a help entry."
	@echo "Run make test-ublox or make test-led, etc."
	@echo "You can get a list with tab complete"
	@echo "or just 'west build -b \$$board tests/ublox'"

LIST = $(subst /,,$(subst tests/,,$(dir $(wildcard tests/*/))))
define make-test-target
.PHONY: test-$1
test-$1:
	$(call buildMacro,$(BOARD), tests/$1)
endef

$(foreach element,$(LIST),$(eval $(call make-test-target,$(element))))
