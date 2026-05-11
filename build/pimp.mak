# You gotta PIMP MY CALC

ifndef NO_PIMP
PLS_IGNORE := $(shell >&2 printf "\e[38;2;255;212;0m     ______\e[38;2;69;173;168m ______\e[38;2;255;212;0m ______\e[38;2;69;173;168m ____\n")
PLS_IGNORE := $(shell >&2 printf "\e[38;2;255;212;0m    / __  /\e[38;2;69;173;168m/ ____/\e[38;2;255;212;0m/_  __/\e[38;2;69;173;168m/   |\n")
PLS_IGNORE := $(shell >&2 printf "\e[38;2;255;212;0m   / /_/ /\e[38;2;69;173;168m/ /__  \e[38;2;255;212;0m  / / \e[38;2;69;173;168m / /| |\n")
PLS_IGNORE := $(shell >&2 printf "\e[38;2;255;212;0m  / __ \/\e[38;2;69;173;168m/ ___/ \e[38;2;255;212;0m  / / \e[38;2;69;173;168m / /_| |\n")
PLS_IGNORE := $(shell >&2 printf "\e[38;2;255;212;0m / /_/ /\e[38;2;69;173;168m/ /___ \e[38;2;255;212;0m  / / \e[38;2;69;173;168m / ___  |\n")
PLS_IGNORE := $(shell >&2 printf "\e[38;2;255;212;0m/_____/\e[38;2;69;173;168m/_____/ \e[38;2;255;212;0m /_/ \e[38;2;69;173;168m /_/   |_|\n")
PLS_IGNORE := $(shell >&2 printf "\n")
PLS_IGNORE := $(shell >&2 printf "\e[32mBuilding U$(UPSILON_VERSION)-E$(EPSILON_VERSION)\n")

ifeq (${PLATFORM},device)
  DISPLAY_TARGET = Numworks $(MODEL)
endif

ifeq (${PLATFORM},simulator)
  DISPLAY_TARGET = $(TARGET) simulator
endif

DISPLAY_TARGET ?= "Undefined"

PLS_IGNORE := $(shell >&2 printf "\e[32m  Targeting $(DISPLAY_TARGET)\n")

ifeq ($(OS),Windows_NT)
    DISPLAY_OS = Windows
    ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
        DISPLAY_OS += amd64
    else
        ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
          DISPLAY_OS += x86
        endif
        ifeq ($(PROCESSOR_ARCHITECTURE),x86)
          DISPLAY_OS += x86
        endif
    endif
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        DISPLAY_OS = Linux
    endif
    ifeq ($(UNAME_S),Darwin)
        DISPLAY_OS = macOS
    endif
    UNAME_P := $(shell uname -m)
    ifeq ($(UNAME_P),x86_64)
          DISPLAY_OS += amd64
    endif
    ifneq ($(filter %86,$(UNAME_P)),)
          DISPLAY_OS += x86
    endif
    ifneq ($(filter arm%,$(UNAME_P)),)
          DISPLAY_OS += arm
    endif
endif

DISPLAY_OS ?= "Unknown"


PLS_IGNORE := $(shell >&2 printf "\e[32m    Building on $(DISPLAY_OS)\n")

PLS_IGNORE := $(shell >&2 printf "\e[0m\e[39m\n")
endif

MAKE += NO_PIMP=1
