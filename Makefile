SHELL := /bin/zsh
CMAKE ?= $(shell command -v cmake)
BUILD_DIR ?= build
CONFIG ?= Debug
JUCE_DIR ?= $(abspath ../carbide/JUCE)
JOBS ?= 6
AU_TYPE ?= aumu
AU_SUBTYPE ?= NT01
AU_MANUFACTURER ?= NTRD
AU_COMPONENT ?= nitride.component
AU_INSTALL_DIR ?= $(HOME)/Library/Audio/Plug-Ins/Components
AU_BUILD_PATH := $(BUILD_DIR)/nitride_artefacts/$(CONFIG)/AU/$(AU_COMPONENT)
PERF_BUILD_DIR ?= build/performance
PERF_ARGS ?= --output "$(PERF_BUILD_DIR)/latest.json"

.PHONY: help configure build test benchmark studies gestures deformation review render-studies render-studies-02 render-studies-03 render-studies-04 render-studies-05 install-au validate-au reload-au standalone clean
help:
	@echo "make build        Build AU + Standalone"
	@echo "make test         Check native UI, automation/state and DSP"
	@echo "make benchmark    Measure Release processor rendering; override PERF_ARGS"
	@echo "make standalone   Build and open the native app"
	@echo "make studies      Build and open the sound comparison app"
	@echo "make gestures     Build and open the playable control-surface study"
	@echo "make deformation  Open the separate visible-object interaction prototype"
	@echo "make review       Open the full dark/orange instrument mockup"
	@echo "make render-studies  Write RMS-matched WAV comparisons under build/sound-studies"
	@echo "make render-studies-02  Write the second comparison round under build/sound-studies-02"
	@echo "make render-studies-03  Compare original and note-coupled phase/delay at 30/70/100%"
	@echo "make render-studies-04  Compare adaptive oscillator coupling against F at 50%"
	@echo "make render-studies-05  Compare G with the extended network stress range"
	@echo "make reload-au    Build, install, and validate AU"
	@echo "Overrides: JUCE_DIR, BUILD_DIR, CONFIG, JOBS, AU_INSTALL_DIR"

configure:
	$(CMAKE) -S . -B "$(BUILD_DIR)" -DCMAKE_BUILD_TYPE=$(CONFIG) -DJUCE_DIR="$(JUCE_DIR)"

build: configure
	$(CMAKE) --build "$(BUILD_DIR)" --config $(CONFIG) --parallel $(JOBS)

test: build
	$(CMAKE) --build "$(BUILD_DIR)" --target test

benchmark:
	$(CMAKE) -S . -B "$(PERF_BUILD_DIR)" -DCMAKE_BUILD_TYPE=Release -DJUCE_DIR="$(JUCE_DIR)"
	$(CMAKE) --build "$(PERF_BUILD_DIR)" --target nitride_perf --parallel $(JOBS)
	"$(PERF_BUILD_DIR)/nitride_perf_artefacts/Release/nitride_perf" $(PERF_ARGS)

install-au: build
	mkdir -p "$(AU_INSTALL_DIR)"
	cp -R "$(AU_BUILD_PATH)" "$(AU_INSTALL_DIR)/"
	touch "$(AU_INSTALL_DIR)/$(AU_COMPONENT)"

validate-au:
	auval -v $(AU_TYPE) $(AU_SUBTYPE) $(AU_MANUFACTURER)

reload-au: install-au validate-au

standalone: build
	open "$(BUILD_DIR)/nitride_artefacts/$(CONFIG)/Standalone/nitride.app"

studies: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target nitride_studies --config $(CONFIG) --parallel $(JOBS)
	open "$(BUILD_DIR)/nitride_studies_artefacts/$(CONFIG)/Nitride Sound Studies.app"

gestures: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target nitride_gestures --config $(CONFIG) --parallel $(JOBS)
	open "$(BUILD_DIR)/nitride_gestures_artefacts/$(CONFIG)/Nitride Gesture Study.app"

deformation: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target nitride_deformation --config $(CONFIG) --parallel $(JOBS)
	open "$(BUILD_DIR)/nitride_deformation_artefacts/$(CONFIG)/Nitride Deformation Study.app"

review: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target nitride_review --config $(CONFIG) --parallel $(JOBS)
	open "$(BUILD_DIR)/nitride_review_artefacts/$(CONFIG)/Nitride Instrument Review.app"

render-studies: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target nitride_study_render --config $(CONFIG) --parallel $(JOBS)
	"$(BUILD_DIR)/nitride_study_render" "$(BUILD_DIR)/sound-studies"

render-studies-02: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target nitride_study_render --config $(CONFIG) --parallel $(JOBS)
	"$(BUILD_DIR)/nitride_study_render" "$(BUILD_DIR)/sound-studies-02" --study-02

render-studies-03: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target nitride_study_render --config $(CONFIG) --parallel $(JOBS)
	"$(BUILD_DIR)/nitride_study_render" "$(BUILD_DIR)/sound-studies-03" --study-03

render-studies-04: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target nitride_study_render --config $(CONFIG) --parallel $(JOBS)
	"$(BUILD_DIR)/nitride_study_render" "$(BUILD_DIR)/sound-studies-04" --study-04

render-studies-05: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target nitride_study_render --config $(CONFIG) --parallel $(JOBS)
	"$(BUILD_DIR)/nitride_study_render" "$(BUILD_DIR)/sound-studies-05" --study-05

clean:
	rm -rf "$(BUILD_DIR)"
