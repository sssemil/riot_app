USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_icmpv6_echo
USEMODULE += gcoap

USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

CFLAGS += -DOSCORE_NANOCOAP_MEMMOVE_MODE

BOARD ?= native

ifneq ($(BOARD),native)
FEATURES_REQUIRED += periph_flashpage
endif

ifeq ($(BOARD),particle-xenon)
# The firmware upload method described in the documentation
PARTICLE_MONOFIRMWARE = 1

# Don't checksum the whole program, that'd render the whole program unbootable
# once data is persisted
CFLAGS += -DPARTICLE_MONOFIRMWARE_CHECKSUMLIMIT

USEMODULE += auto_init_usbus
USEMODULE += stdio_cdc_acm
USEMODULE += usbus_cdc_ecm
GNRC_NETIF_NUMOF = 2
CFLAGS += -DGNRC_IPV6_NIB_CONF_SLAAC -DGNRC_IPV6_NIB_CONF_ARSM
# Allow full-length lines even over CDC-ACM
CFLAGS += -DUSBUS_CDC_ACM_STDIO_BUF_SIZE=512
endif

ifeq ($(BOARD),iotlab-m3)
# for IoT lab
GNRC_NETIF_NUMOF := 2
USEMODULE += ethos
# Those seem to be out of date
#ETHOS_BAUDRATE = 500000
#CFLAGS += -DETHOS_BAUDRATE=$(ETHOS_BAUDRATE)

USEMODULE += stdio_ethos
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_sixlowpan_border_router_default
USEMODULE += gnrc_icmpv6_echo
USEMODULE += gnrc_uhcpc
USEMODULE += fib
# to allow adding a global address to the ethos interface as well
CFLAGS += -DGNRC_NETIF_IPV6_ADDRS_NUMOF=3
endif

# The +1500 it's still a but much for my taste, and I should confirm my suspicion about
# most of it being the crypto library at some point.
CFLAGS += -DGCOAP_STACK_SIZE="THREAD_STACKSIZE_DEFAULT + DEBUG_EXTRA_STACKSIZE + sizeof(coap_pkt_t) + 1500"
# As the main thread sends messages, it needs an increased size.
CFLAGS += -DTHREAD_STACKSIZE_MAIN="THREAD_STACKSIZE_DEFAULT + THREAD_EXTRA_STACKSIZE_PRINTF + 1500"

# Make sanely sized blocks
CFLAGS += -DGCOAP_PDU_BUF_SIZE=600

APPLICATION ?= test_$(notdir $(subst -,_,$(patsubst %/,%,$(CURDIR))))

RIOTBASE ?= $(CURDIR)/../../../RIOT

BOARD ?= native

DEVELHELP ?= 1

# C11 features are accepted in the OSCORE implementation, but that causes
# pedantic Werrors to show up in RIOT source; gnu11 seems to ignore those.
CFLAGS += -std=gnu11

EXTERNAL_MODULE_DIRS += $(abspath ../../../..)/liboscore/backends/riot-module
USEMODULE += oscore

# Pick a backend; without, no algorithms are available
USEMODULE += libcose_crypt_hacl
# That would offer AES-CCM algorithms, but using it requires changing the RIOT
# package to point to the branch at https://github.com/bergzand/libcose/pull/91
# until that is completed and a version including it has been mada available in
# RIOT.
# USEMODULE += libcose_crypt_tinydtls

#EXTERNAL_MODULE_DIRS += $(abspath ..)/testwrapper

#TESTS = ../test-evaluator.sh

# workarounds for https://github.com/RIOT-OS/RIOT/issues/13121
USEMODULE += gnrc_sock
USEMODULE += gnrc_ipv6

include $(RIOTBASE)/Makefile.include

