/*	$OpenBSD: src/sys/dev/pci/maestro.c,v 1.4 2001/03/13 01:45:56 deraadt Exp $	*/
/* $FreeBSD: /c/ncvs/src/sys/dev/sound/pci/maestro.c,v 1.3 2000/11/21 12:22:11 julian Exp $ */
/*
 * FreeBSD's ESS Agogo/Maestro driver 
 * Converted from FreeBSD's pcm to OpenBSD's audio.
 * Copyright (c) 2000, 2001 David Leonard & Marc Espie
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*-
 * (FreeBSD) Credits:
 * Copyright (c) 2000 Taku YAMAMOTO <taku@cent.saitama-u.ac.jp>
 *
 * Part of this code (especially in many magic numbers) was heavily inspired
 * by the Linux driver originally written by
 * Alan Cox <alan.cox@linux.org>, modified heavily by
 * Zach Brown <zab@zabbo.net>.
 *
 * busdma()-ize and buffer size reduction were suggested by
 * Cameron Grant <gandalf@vilnya.demon.co.uk>.
 * Also he showed me the way to use busdma() suite.
 *
 * Internal speaker problems on NEC VersaPro's and Dell Inspiron 7500
 * were looked at by
 * Munehiro Matsuda <haro@tk.kubota.co.jp>,
 * who brought patches based on the Linux driver with some simplification.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/fcntl.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/mulaw.h>
#include <dev/auconv.h>

#include <dev/ic/ac97.h>
#include <dev/pci/maestro_reg.h>

#ifdef AUDIO_DEBUG
#define DPRINTF(x)	if (maestrodebug) printf x
#define DLPRINTF(i, x)	if (maestrodebug & i) printf x
int	maestrodebug = 0;
u_long maestrointr_called;
u_long maestrodma_effective;

#define MAESTRODEBUG_INTR 1
#define MAESTRODEBUG_TIMER 2
#else
#define DPRINTF(x)
#define DLPRINTF(i, x)
#endif

#define MAESTRO_BUFSIZ		0x4000
#define lengthof(array)		(sizeof (array) / sizeof (array)[0])

#define STEP_VOLUME		0x22
#define MIDDLE_VOLUME		(STEP_VOLUME * 4)

typedef struct salloc_pool {
	struct salloc_zone {
		SLIST_ENTRY(salloc_zone) link;
		caddr_t		addr;
		size_t		size;
	} *zones;
	SLIST_HEAD(salloc_head, salloc_zone) free, used, spare;
} *salloc_t;

struct maestro_softc;

#define MAESTRO_PLAY	1
#define MAESTRO_STEREO	2
#define MAESTRO_8BIT	4
#define MAESTRO_UNSIGNED	8
#define MAESTRO_RUNNING	16

struct maestro_channel {
	struct maestro_softc 	*sc;
	int			num;
	u_int32_t		blocksize;
	u_int16_t		mode;
	u_int32_t		speed;
	u_int32_t		dv;
	u_int16_t		start;
	u_int16_t		threshold;
	u_int16_t		end;
	u_int16_t		current;
	u_int			wpwa;
	void			(*intr) __P((void *));
	void			*intr_arg;
};

struct maestro_softc {
	struct device		dev;

	void			*ih;
	pci_chipset_tag_t	pc;
	pcitag_t		pt;

#define MAESTRO_FLAG_SETUPGPIO	0x0001
	int			flags;
	bus_space_tag_t		iot;
	bus_space_handle_t	ioh;
	bus_dma_tag_t		dmat;

	caddr_t			dmabase;
	bus_addr_t		physaddr;
	size_t			dmasize;
	bus_dmamap_t		dmamap;
	bus_dma_segment_t	dmaseg;
	salloc_t		dmapool;

	struct ac97_codec_if	*codec_if;
	struct ac97_host_if	host_if;
	struct audio_device	*sc_audev;

	void			*powerhook;
	int			suspend;

	struct maestro_channel	play;
	struct maestro_channel	record;
};


typedef	u_int16_t wpreg_t;
typedef	u_int16_t wcreg_t;

salloc_t salloc_new __P((caddr_t, size_t, int));
void	salloc_destroy __P((salloc_t));
caddr_t	salloc_alloc __P((salloc_t, size_t));
void	salloc_free __P((salloc_t, caddr_t));
void	salloc_insert __P((salloc_t, struct salloc_head *, 
		struct salloc_zone *, int));

int	maestro_match __P((struct device *, void *, void *));
void	maestro_attach __P((struct device *, struct device *, void *));
int	maestro_intr __P((void *));

int	maestro_open __P((void *, int));
void	maestro_close __P((void *));
int	maestro_query_encoding __P((void *, struct audio_encoding *));
int	maestro_set_params __P((void *, int, int, struct audio_params *, 
			    struct audio_params *));
int	maestro_round_blocksize __P((void *, int));
int	maestro_halt_output __P((void *));
int	maestro_halt_input __P((void *));
int	maestro_getdev __P((void *, struct audio_device *));
int	maestro_set_port __P((void *, mixer_ctrl_t *));
int	maestro_get_port __P((void *, mixer_ctrl_t *));
int	maestro_query_devinfo __P((void *, mixer_devinfo_t *));
void	*maestro_malloc __P((void *, int, size_t, int, int));
void	maestro_free __P((void *, void *, int));
size_t	maestro_round_buffersize __P((void *, int, size_t));
int	maestro_mappage __P((void *, void *, int, int));
int	maestro_get_props __P((void *));
int	maestro_trigger_output __P((void *, void *, void *, int, void (*)(void *),
				void *, struct audio_params *));
int	maestro_trigger_input __P((void *, void *, void *, int, void (*)(void *),
			       void *, struct audio_params *));

int	maestro_attach_codec __P((void *, struct ac97_codec_if *));
int	maestro_read_codec __P((void *, u_int8_t, u_int16_t *));
int	maestro_write_codec __P((void *, u_int8_t, u_int16_t));
void	maestro_reset_codec __P((void *));

void	maestro_initcodec __P((void *));

void	maestro_set_speed __P((struct maestro_channel *, u_long *));
void	maestro_init __P((struct maestro_softc *));
void	maestro_power __P((struct maestro_softc *, int));
void	maestro_powerhook __P((int, void *));

void 	maestro_channel_start __P((struct maestro_channel *));
void 	maestro_channel_stop __P((struct maestro_channel *));
void 	maestro_channel_advance_dma __P((struct maestro_channel *));

int	maestro_get_flags __P((struct pci_attach_args *));

void	ringbus_setdest __P((struct maestro_softc *, int, int));

wpreg_t	wp_reg_read __P((struct maestro_softc *, int));
void	wp_reg_write __P((struct maestro_softc *, int, wpreg_t));
wpreg_t	wp_apu_read __P((struct maestro_softc *, int, int));
void	wp_apu_write __P((struct maestro_softc *, int, int, wpreg_t));
void	wp_settimer __P((struct maestro_softc *, u_int));
void	wp_starttimer __P((struct maestro_softc *));
void	wp_stoptimer __P((struct maestro_softc *));

wcreg_t	wc_reg_read __P((struct maestro_softc *, int));
void	wc_reg_write __P((struct maestro_softc *, int, wcreg_t));
wcreg_t	wc_ctrl_read __P((struct maestro_softc *, int));
void	wc_ctrl_write __P((struct maestro_softc *, int, wcreg_t));

u_int maestro_calc_timer_freq __P((struct maestro_channel *));
void maestro_update_timer __P((struct maestro_softc *));

struct cfdriver maestro_cd = {
	NULL, "maestro", DV_DULL
};

struct cfattach maestro_ca = {
	sizeof (struct maestro_softc), maestro_match, maestro_attach
};

struct audio_hw_if maestro_hw_if = {
	maestro_open,
	maestro_close,
	NULL,
	maestro_query_encoding,
	maestro_set_params,
	maestro_round_blocksize,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	maestro_halt_output,
	maestro_halt_input,
	NULL,
	maestro_getdev,
	NULL,
	maestro_set_port,
	maestro_get_port,
	maestro_query_devinfo,
	NULL,
	maestro_free,
	NULL,
	maestro_mappage,
	maestro_get_props,
	maestro_trigger_output,
	maestro_trigger_input,
	maestro_malloc,
	maestro_round_buffersize
};

struct audio_device maestro_audev = {
	"ESS Maestro", "", "maestro"
};

struct {
	u_short vendor, product;
	int flags;
} maestro_pcitab[] = {
	{ PCI_VENDOR_ESSTECH, PCI_PRODUCT_ESSTECH_MAESTROII,	0 },
	{ PCI_VENDOR_ESSTECH, PCI_PRODUCT_ESSTECH_MAESTRO2E,	0 },
	{ PCI_VENDOR_PLATFORM, PCI_PRODUCT_PLATFORM_ES1849,	0 },
	{ PCI_VENDOR_NEC, PCI_PRODUCT_NEC_VERSAMAESTRO,		MAESTRO_FLAG_SETUPGPIO },
	{ PCI_VENDOR_NEC, PCI_PRODUCT_NEC_VERSAPRONXVA26D,	MAESTRO_FLAG_SETUPGPIO }
};
#define NMAESTRO_PCITAB	lengthof(maestro_pcitab)

int
maestro_get_flags(pa)
	struct pci_attach_args *pa;
{
	int i;

	/* Distinguish audio devices from modems with the same manfid */
	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_MULTIMEDIA)
		return (-1);
	if (PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_MULTIMEDIA_AUDIO)
		return (-1);
	for (i = 0; i < NMAESTRO_PCITAB; i++)
		if (PCI_VENDOR(pa->pa_id) == maestro_pcitab[i].vendor &&
		    PCI_PRODUCT(pa->pa_id) == maestro_pcitab[i].product)
			return (maestro_pcitab[i].flags);
	return (-1);
}

/* -----------------------------
 * Driver interface.
 */

int
maestro_match(parent, match, aux)
	struct device *parent;
	void *match;
	void *aux;
{
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;

	if (maestro_get_flags(pa) == -1)
		return (0);
	else
		return (1);
}

void
maestro_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct maestro_softc *sc = (struct maestro_softc *)self;
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	char const *intrstr;
	pci_intr_handle_t ih;
	int error;
	pcireg_t data;
	u_int16_t cdata;
	int dmastage = 0;
	int rseg;

	sc->sc_audev = &maestro_audev;
	sc->flags = maestro_get_flags(pa);

	sc->pc = pa->pa_pc;
	sc->pt = pa->pa_tag;
	sc->dmat = pa->pa_dmat;

	/* Map interrupt */
	if (pci_intr_map(pc, pa->pa_intrtag, pa->pa_intrpin, pa->pa_intrline,
	    &ih)) {
		printf(": couldn't map interrupt\n");
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->ih = pci_intr_establish(pc, ih, IPL_AUDIO, maestro_intr, sc,
	    sc->dev.dv_xname);
	if (sc->ih == NULL) {
		printf(": couldn't establish interrupt");
		if (intrstr != NULL)
			printf(" at %s\n", intrstr);
		return;
	}
	printf(": %s", intrstr);

	/* Rangers, power up */
	maestro_power(sc, PPMI_D0);
	DELAY(100000);

	/* Map i/o */
	if ((error = pci_mapreg_map(pa, PCI_MAPS, PCI_MAPREG_TYPE_IO, 
	    0, &sc->iot, &sc->ioh, NULL, NULL)) != 0) {
		printf(", couldn't map i/o space\n");
		goto bad;
	};

	/* Enable bus mastering */
	data = pci_conf_read(sc->pc, sc->pt, PCI_COMMAND_STATUS_REG);
	if ((data & PCI_COMMAND_MASTER_ENABLE) == 0)
		pci_conf_write(sc->pc, sc->pt, PCI_COMMAND_STATUS_REG,
			data | PCI_COMMAND_MASTER_ENABLE);

	/* Allocate fixed DMA segment :-( */
	sc->dmasize = MAESTRO_BUFSIZ * 16;
	if ((error = bus_dmamem_alloc(sc->dmat, sc->dmasize, NBPG, 0, 
	    &sc->dmaseg, 1, &rseg, BUS_DMA_NOWAIT)) != 0) {
		printf(", unable to alloc dma, error %d\n", error);
		goto bad;
	}
	dmastage = 1;
	if ((error = bus_dmamem_map(sc->dmat, &sc->dmaseg, 1,
	    sc->dmasize, &sc->dmabase, BUS_DMA_NOWAIT | 
	    BUS_DMA_COHERENT)) != 0) {
		printf(", unable to map dma, error %d\n", error);
		goto bad;
	}
	dmastage = 2;
	if ((error = bus_dmamap_create(sc->dmat, sc->dmasize, 1, 
	    sc->dmasize, 0, BUS_DMA_NOWAIT, &sc->dmamap)) != 0) {
		printf(", unable to create dma map, error %d\n", error);
		goto bad;
	}
	dmastage = 3;
	if ((error = bus_dmamap_load(sc->dmat, sc->dmamap, 
	    sc->dmabase, sc->dmasize, NULL, BUS_DMA_NOWAIT)) != 0) {
		printf(", unable to load dma map, error %d\n", error);
		goto bad;
	}

	/* XXX 
	 * The first byte of the allocated memory is not usable,
	 * the WP sometimes uses it to store status.
	 */
	/* Make DMA memory pool */
	if ((sc->dmapool = salloc_new(sc->dmabase+16, sc->dmasize-16,
	    128/*overkill?*/)) == NULL) {
		printf(", unable to make dma pool\n");
		goto bad;
	}
	
	sc->physaddr = sc->dmamap->dm_segs[0].ds_addr;

	printf("\n");

	/* Kick device */
	maestro_init(sc);
	maestro_read_codec(sc, 0, &cdata);
	if (cdata == 0x80) {
		printf("%s: PT101 codec unsupported, no mixer\n", 
		    sc->dev.dv_xname);
		/* Init values from Linux, no idea what this does. */
		maestro_write_codec(sc, 0x2a, 0x0001);
		maestro_write_codec(sc, 0x2C, 0x0000);
		maestro_write_codec(sc, 0x2C, 0xFFFF);
		maestro_write_codec(sc, 0x10, 0x9F1F);
		maestro_write_codec(sc, 0x12, 0x0808);
		maestro_write_codec(sc, 0x14, 0x9F1F);
		maestro_write_codec(sc, 0x16, 0x9F1F);
		maestro_write_codec(sc, 0x18, 0x0404);
		maestro_write_codec(sc, 0x1A, 0x0000);
		maestro_write_codec(sc, 0x1C, 0x0000);
		maestro_write_codec(sc, 0x02, 0x0404);
		maestro_write_codec(sc, 0x04, 0x0808);
		maestro_write_codec(sc, 0x0C, 0x801F);
		maestro_write_codec(sc, 0x0E, 0x801F);
		/* no control over the mixer, sorry */
		sc->codec_if = NULL;
	} else {
		/* Attach the AC'97 */
		sc->host_if.arg = sc;
		sc->host_if.attach = maestro_attach_codec;
		sc->host_if.read = maestro_read_codec;
		sc->host_if.write = maestro_write_codec;
		sc->host_if.reset = maestro_reset_codec;
		if (ac97_attach(&sc->host_if) != 0) {
			printf("%s: couldn't attach codec\n", sc->dev.dv_xname);
			goto bad;
		}
	}

	sc->play.mode = MAESTRO_PLAY;
	sc->play.sc = sc;
	sc->play.num = 0;
	sc->record.sc = sc;
	sc->record.num = 2;
	sc->record.mode = 0;

	/* Attach audio */
	audio_attach_mi(&maestro_hw_if, sc, &sc->dev);

	/* Hook power changes */
	sc->suspend = PWR_RESUME;
	sc->powerhook = powerhook_establish(maestro_powerhook, sc);

	return;

 bad:
	/* Power down. */
	maestro_power(sc, PPMI_D3);
	if (sc->ih)
		pci_intr_disestablish(pc, sc->ih);
	printf("%s: disabled\n", sc->dev.dv_xname);
	if (sc->dmapool)
		salloc_destroy(sc->dmapool);
	if (dmastage >= 3)
		bus_dmamap_destroy(sc->dmat, sc->dmamap);
	if (dmastage >= 2)
		bus_dmamem_unmap(sc->dmat, sc->dmabase, sc->dmasize);
	if (dmastage >= 1)
		bus_dmamem_free(sc->dmat, &sc->dmaseg, 1);
}

void
maestro_init(sc)
	struct maestro_softc *sc;
{
	int reg;
	pcireg_t data;

	/* Disable all legacy emulations. */
	data = pci_conf_read(sc->pc, sc->pt, CONF_LEGACY);
	data |= LEGACY_DISABLED;
	pci_conf_write(sc->pc, sc->pt, CONF_LEGACY, data);

	/* Disconnect from CHI. (Makes Dell inspiron 7500 work?)
	 * Enable posted write.
	 * Prefer PCI timing rather than that of ISA.
	 * Don't swap L/R. */
	data = pci_conf_read(sc->pc, sc->pt, CONF_MAESTRO);
	data |= MAESTRO_CHIBUS | MAESTRO_POSTEDWRITE | MAESTRO_DMA_PCITIMING;
	data &= ~MAESTRO_SWAP_LR;
	pci_conf_write(sc->pc, sc->pt, CONF_MAESTRO, data);
	/* Reset direct sound. */
	bus_space_write_2(sc->iot, sc->ioh, PORT_HOSTINT_CTRL,
	    HOSTINT_CTRL_DSOUND_RESET);
	DELAY(10000);	/* XXX - too long? */
	bus_space_write_2(sc->iot, sc->ioh, PORT_HOSTINT_CTRL, 0);
	DELAY(10000);

	/* Enable direct sound and hardware volume control interruptions. */
	bus_space_write_2(sc->iot, sc->ioh, PORT_HOSTINT_CTRL,
	    HOSTINT_CTRL_DSOUND_INT_ENABLED | HOSTINT_CTRL_HWVOL_ENABLED);

	/* Setup Wave Processor. */

	/* Enable WaveCache, set DMA base address. */
	wp_reg_write(sc, WPREG_WAVE_ROMRAM,
	    WP_WAVE_VIRTUAL_ENABLED | WP_WAVE_DRAM_ENABLED);
	bus_space_write_2(sc->iot, sc->ioh, PORT_WAVCACHE_CTRL,
	    WAVCACHE_ENABLED | WAVCACHE_WTSIZE_4MB);

	for (reg = WAVCACHE_PCMBAR; reg < WAVCACHE_PCMBAR + 4; reg++)
		wc_reg_write(sc, reg, 
			sc->physaddr >> WAVCACHE_BASEADDR_SHIFT);

	/* Setup Codec/Ringbus. */
	maestro_initcodec(sc);
	bus_space_write_4(sc->iot, sc->ioh, PORT_RINGBUS_CTRL,
	    RINGBUS_CTRL_RINGBUS_ENABLED | RINGBUS_CTRL_ACLINK_ENABLED);

	wp_reg_write(sc, WPREG_BASE, 0x8500);	/* Parallel I/O */
	ringbus_setdest(sc, RINGBUS_SRC_ADC,
	    RINGBUS_DEST_STEREO | RINGBUS_DEST_DSOUND_IN);
	ringbus_setdest(sc, RINGBUS_SRC_DSOUND,
	    RINGBUS_DEST_STEREO | RINGBUS_DEST_DAC);

	/* Setup ASSP. Needed for Dell Inspiron 7500? */
	bus_space_write_1(sc->iot, sc->ioh, PORT_ASSP_CTRL_B, 0x00);
	bus_space_write_1(sc->iot, sc->ioh, PORT_ASSP_CTRL_A, 0x03);
	bus_space_write_1(sc->iot, sc->ioh, PORT_ASSP_CTRL_C, 0x00);

	/* 
	 * Reset hw volume to a known value so that we may handle diffs
	 * off to AC'97.
	 */

	bus_space_write_1(sc->iot, sc->ioh, PORT_HWVOL_MASTER, MIDDLE_VOLUME);
	/* Setup GPIO if needed (NEC systems) */
	if (sc->flags & MAESTRO_FLAG_SETUPGPIO) {
		/* Matthew Braithwaite <matt@braithwaite.net> reported that
		 * NEC Versa LX doesn't need GPIO operation. */
		bus_space_write_2(sc->iot, sc->ioh, 
		    PORT_GPIO_MASK, 0x9ff);
		bus_space_write_2(sc->iot, sc->ioh, PORT_GPIO_DIR,
		    bus_space_read_2(sc->iot, sc->ioh, PORT_GPIO_DIR) | 0x600);
		bus_space_write_2(sc->iot, sc->ioh, 
		    PORT_GPIO_DATA, 0x200);
	}
}

/* -----------------------------
 * Audio interface
 */

int
maestro_round_blocksize(self, blk)
	void *self;
	int blk;
{
	return (blk & ~0xf);
}

size_t
maestro_round_buffersize(self, direction, size)
	void *self;
	int direction;
	size_t size;
{
	return (size);
}

void *
maestro_malloc(arg, dir, size, pool, flags)
	void *arg;
	int dir;
	size_t size;
	int pool, flags;
{
	struct maestro_softc *sc = (struct maestro_softc *)arg;

	return (salloc_alloc(sc->dmapool, size));
}

void
maestro_free(self, ptr, pool)
	void *self, *ptr;
	int pool;
{
	struct maestro_softc *sc = (struct maestro_softc *)self;

	salloc_free(sc->dmapool, ptr);
}

int
maestro_mappage(self, mem, off, prot)
	void *self, *mem;
	int off, prot;
{
	struct maestro_softc *sc = (struct maestro_softc *)self;

	if (off < 0)
		return -1;
	return bus_dmamem_mmap(sc->dmat, &sc->dmaseg, 1,
	    (caddr_t)mem - sc->dmabase + off, prot, BUS_DMA_WAITOK);
}

int
maestro_get_props(self)
	void *self;
{
	/* struct maestro_softc *sc = (struct maestro_softc *)self; */

	return (AUDIO_PROP_MMAP | AUDIO_PROP_INDEPENDENT); /* XXX */
}

int
maestro_getdev(self, retp)
	void *self;
	struct audio_device *retp;
{
	struct maestro_softc *sc = (struct maestro_softc *)self;

	*retp = *sc->sc_audev;
	return 0;
}

int
maestro_set_port(self, cp)
	void *self;
	mixer_ctrl_t *cp;
{
	struct ac97_codec_if *c = ((struct maestro_softc *)self)->codec_if;

	if (c)
		return (c->vtbl->mixer_set_port(c, cp));
	else
		return (ENXIO);
}

int
maestro_get_port(self, cp)
	void *self;
	mixer_ctrl_t *cp;
{
	struct ac97_codec_if *c = ((struct maestro_softc *)self)->codec_if;

	if (c)
		return (c->vtbl->mixer_get_port(c, cp));
	else
		return (ENXIO);
}

int
maestro_query_devinfo(self, cp)
	void *self;
	mixer_devinfo_t *cp;
{
	struct ac97_codec_if *c = ((struct maestro_softc *)self)->codec_if;

	if (c)
		return (c->vtbl->query_devinfo(c, cp));
	else
		return (ENXIO);
}

struct audio_encoding maestro_tab[] = { 
	{0, AudioEslinear_le, AUDIO_ENCODING_SLINEAR_LE, 16, 0},
	{1, AudioEslinear, AUDIO_ENCODING_SLINEAR, 8, 0},
	{2, AudioEulinear, AUDIO_ENCODING_ULINEAR, 8, 0},
	{3, AudioEslinear_be, AUDIO_ENCODING_SLINEAR_BE, 16,
	    AUDIO_ENCODINGFLAG_EMULATED},
	{4, AudioEulinear_le, AUDIO_ENCODING_ULINEAR_LE, 16,
	    AUDIO_ENCODINGFLAG_EMULATED},
	{5, AudioEulinear_be, AUDIO_ENCODING_ULINEAR_BE, 16,
	    AUDIO_ENCODINGFLAG_EMULATED},
	{6, AudioEmulaw, AUDIO_ENCODING_ULAW, 8,
	    AUDIO_ENCODINGFLAG_EMULATED},
	{7, AudioEalaw, AUDIO_ENCODING_ALAW, 8,
	    AUDIO_ENCODINGFLAG_EMULATED}
};

int
maestro_query_encoding(hdl, fp)
	void *hdl;
	struct audio_encoding *fp;
{
	if (fp->index < 0 || fp->index >= lengthof(maestro_tab))
		return (EINVAL);
	*fp = maestro_tab[fp->index];
	return (0);
}

#define UNUSED __attribute__((unused))

void
maestro_set_speed(ch, prate)
	struct maestro_channel *ch;
	u_long *prate;
{
	ch->speed = *prate;
	if ((ch->mode & (MAESTRO_8BIT | MAESTRO_STEREO)) == MAESTRO_8BIT)
		ch->speed /= 2;
		
	/* special common case */
	if (ch->speed == 48000) {
		ch->dv = 0x10000;
	} else {
		/* compute 16 bits fixed point value of speed/48000,
		 * being careful not to overflow */
		 ch->dv = (((ch->speed % 48000) << 16U) + 24000) / 48000
		    + ((ch->speed / 48000) << 16U);
		/* And this is the real rate obtained */
		ch->speed = (ch->dv >> 16U) * 48000 + 
		    (((ch->dv & 0xffff)*48000)>>16U);
	}
	*prate = ch->speed;
	if ((ch->mode & (MAESTRO_8BIT | MAESTRO_STEREO)) == MAESTRO_8BIT)
		*prate *= 2;
}

u_int
maestro_calc_timer_freq(ch)
	struct maestro_channel *ch;
{
	u_int	ss = 2;

	if (ch->mode & MAESTRO_8BIT)
		ss = 1;
	return (ch->speed * ss) / ch->blocksize;
}

void
maestro_update_timer(sc)
	struct maestro_softc *sc;
{
	u_int freq = 0;
	u_int n;

	if (sc->play.mode & MAESTRO_RUNNING)
		freq = maestro_calc_timer_freq(&sc->play);
	if (sc->record.mode & MAESTRO_RUNNING) {
		n = maestro_calc_timer_freq(&sc->record);
		if (freq < n)
			freq = n;
	}
	if (freq) {
		wp_settimer(sc, freq);
		wp_starttimer(sc);
    	} else
		wp_stoptimer(sc);
}


int
maestro_set_params(hdl, setmode, usemode, play, rec)
	void *hdl;
	int setmode, usemode;
	struct audio_params *play, *rec;
{
	struct maestro_softc *sc = (struct maestro_softc *)hdl;
	
	if ((setmode & AUMODE_PLAY) == 0)
		return (0);

	/* Disallow parameter change on a running audio for now */
	if (sc->play.mode & MAESTRO_RUNNING)
		return (EINVAL);

	if (play->sample_rate < 4000)
		play->sample_rate = 4000;
	else if (play->sample_rate > 48000)
		play->sample_rate = 48000;

	play->factor = 1;
	play->sw_code = NULL;
	if (play->channels != 1 && play->channels != 2)
		return (EINVAL);


	sc->play.mode = MAESTRO_PLAY;
	if (play->channels == 2)
		sc->play.mode |= MAESTRO_STEREO;

	if (play->encoding == AUDIO_ENCODING_ULAW) {
		play->factor = 2;
		play->sw_code = mulaw_to_slinear16_le;
	} else if (play->encoding == AUDIO_ENCODING_ALAW) {
		play->factor = 2;
		play->sw_code = alaw_to_slinear16_le;
	} else if (play->precision == 8) {
		sc->play.mode |= MAESTRO_8BIT;
		if (play->encoding == AUDIO_ENCODING_ULINEAR_LE ||
		    play->encoding == AUDIO_ENCODING_ULINEAR_BE)
		    sc->play.mode |= MAESTRO_UNSIGNED;
	}
	else if (play->encoding == AUDIO_ENCODING_ULINEAR_LE)
		play->sw_code = change_sign16_le;
	else if (play->encoding == AUDIO_ENCODING_SLINEAR_BE)
		play->sw_code = swap_bytes;
	else if (play->encoding == AUDIO_ENCODING_ULINEAR_BE)
		play->sw_code = change_sign16_swap_bytes_le;
	else if (play->encoding != AUDIO_ENCODING_SLINEAR_LE)
		return (EINVAL);

	maestro_set_speed(&sc->play, &play->sample_rate);
	return (0);
}

int
maestro_open(hdl, flags)
	void *hdl;
	int flags;
{
	struct maestro_softc *sc = (struct maestro_softc *)hdl;
	DPRINTF(("%s: open(%d)\n", sc->dev.dv_xname, flags));

	if ((OFLAGS(flags) & O_ACCMODE) != O_WRONLY)
		return (EINVAL);
	sc->play.mode = MAESTRO_PLAY;
	sc->record.mode = 0;
#ifdef AUDIO_DEBUG
	maestrointr_called = 0;
	maestrodma_effective = 0;
#endif
	return (0);
}

void
maestro_close(hdl)
	void *hdl;
{
	struct maestro_softc *sc UNUSED = (struct maestro_softc *)hdl;
	/* nothing to do */
}


void
maestro_channel_stop(ch)
	struct maestro_channel *ch;
{
	wp_apu_write(ch->sc, ch->num, APUREG_APUTYPE,
	    APUTYPE_INACTIVE << APU_APUTYPE_SHIFT);
	if (ch->mode & MAESTRO_STEREO)
	    wp_apu_write(ch->sc, ch->num+1, APUREG_APUTYPE,
		APUTYPE_INACTIVE << APU_APUTYPE_SHIFT);
	/* four channels for record... */
	if (ch->mode & MAESTRO_PLAY)
		return;
	wp_apu_write(ch->sc, ch->num+2, APUREG_APUTYPE,
	    APUTYPE_INACTIVE << APU_APUTYPE_SHIFT);
	if (ch->mode & MAESTRO_STEREO)
	    wp_apu_write(ch->sc, ch->num+3, APUREG_APUTYPE,
		APUTYPE_INACTIVE << APU_APUTYPE_SHIFT);
	
}

int
maestro_halt_input(hdl)
	void *hdl;
{
	struct maestro_softc *sc = (struct maestro_softc *)hdl;
	maestro_channel_stop(&sc->record);
	sc->record.mode &= ~MAESTRO_RUNNING;
	maestro_update_timer(sc);
	return 0;
}

int
maestro_halt_output(hdl)
	void *hdl;
{
	struct maestro_softc *sc = (struct maestro_softc *)hdl;

	maestro_channel_stop(&sc->play);
	sc->play.mode &= ~MAESTRO_RUNNING;
	maestro_update_timer(sc);
	return 0;
}

int
maestro_trigger_input(hdl, start, end, blksize, intr, arg, param)
	void *hdl;
	void *start, *end;
	int blksize;
	void (*intr) __P((void *));
	void *arg;
	struct audio_params *param;
{
	struct maestro_softc *sc = (struct maestro_softc *)hdl;

	sc->record.mode |= MAESTRO_RUNNING;

	maestro_channel_start(&sc->record);

	sc->record.threshold = sc->record.start;
	maestro_update_timer(sc);
	return 0;
}

void
maestro_channel_start(ch)
	struct maestro_channel *ch;
{
	struct maestro_softc *sc = ch->sc;
	int n = ch->num;
	int aputype;
	wcreg_t wcreg = (sc->physaddr - 16) & WAVCACHE_CHCTL_ADDRTAG_MASK;

	switch(ch->mode & (MAESTRO_STEREO | MAESTRO_8BIT)) {
	case 0:
		aputype = APUTYPE_16BITLINEAR;
		break;
	case MAESTRO_STEREO:
		aputype = APUTYPE_16BITSTEREO;
		break;
	case MAESTRO_8BIT:
		aputype = APUTYPE_8BITLINEAR;
		break;
	case MAESTRO_8BIT|MAESTRO_STEREO:
		aputype = APUTYPE_8BITSTEREO;
		break;
	}
	if (ch->mode & MAESTRO_UNSIGNED)
		wcreg |= WAVCACHE_CHCTL_U8;
	if ((ch->mode & MAESTRO_STEREO) == 0) {
		DPRINTF(("Setting mono parameters\n"));
		wp_apu_write(sc, n, APUREG_WAVESPACE, ch->wpwa & 0xff00);
		wp_apu_write(sc, n, APUREG_CURPTR, ch->current);
		wp_apu_write(sc, n, APUREG_ENDPTR, ch->end);
		wp_apu_write(sc, n, APUREG_LOOPLEN, ch->end - ch->start);
		wp_apu_write(sc, n, APUREG_AMPLITUDE, 0xe800);
		wp_apu_write(sc, n, APUREG_POSITION, 0x8f00
		    | (RADIUS_CENTERCIRCLE << APU_RADIUS_SHIFT)
		    | (PAN_FRONT << APU_PAN_SHIFT));
		wp_apu_write(sc, n, APUREG_FREQ_LOBYTE, APU_plus6dB
		    | ((ch->dv & 0xff) << APU_FREQ_LOBYTE_SHIFT));
		wp_apu_write(sc, n, APUREG_FREQ_HIWORD, ch->dv >> 8);
		wc_ctrl_write(sc, n, wcreg);
		wp_apu_write(sc, n, APUREG_APUTYPE,
		    (aputype << APU_APUTYPE_SHIFT) | APU_DMA_ENABLED | 0xf);
	} else {
		wcreg |= WAVCACHE_CHCTL_STEREO;
		DPRINTF(("Setting stereo parameters\n"));
		wp_apu_write(sc, n+1, APUREG_WAVESPACE, ch->wpwa & 0xff00);
		wp_apu_write(sc, n+1, APUREG_CURPTR, ch->current);
		wp_apu_write(sc, n+1, APUREG_ENDPTR, ch->end);
		wp_apu_write(sc, n+1, APUREG_LOOPLEN, ch->end - ch->start);
		wp_apu_write(sc, n+1, APUREG_AMPLITUDE, 0xe800);
		wp_apu_write(sc, n+1, APUREG_POSITION, 0x8f00
		    | (RADIUS_CENTERCIRCLE << APU_RADIUS_SHIFT)
		    | (PAN_LEFT << APU_PAN_SHIFT));
		wp_apu_write(sc, n+1, APUREG_FREQ_LOBYTE, APU_plus6dB
		    | ((ch->dv & 0xff) << APU_FREQ_LOBYTE_SHIFT));
		wp_apu_write(sc, n+1, APUREG_FREQ_HIWORD, ch->dv >> 8);
		if (ch->mode & MAESTRO_8BIT)
			wp_apu_write(sc, n, APUREG_WAVESPACE, 
			    ch->wpwa & 0xff00);
		    else
			wp_apu_write(sc, n, APUREG_WAVESPACE, 
			    (ch->wpwa|(APU_STEREO >> 1)) & 0xff00);
		wp_apu_write(sc, n, APUREG_CURPTR, ch->current);
		wp_apu_write(sc, n, APUREG_ENDPTR, ch->end);
		wp_apu_write(sc, n, APUREG_LOOPLEN, ch->end - ch->start);
		wp_apu_write(sc, n, APUREG_AMPLITUDE, 0xe800);
		wp_apu_write(sc, n, APUREG_POSITION, 0x8f00
		    | (RADIUS_CENTERCIRCLE << APU_RADIUS_SHIFT)
		    | (PAN_RIGHT << APU_PAN_SHIFT));
		wp_apu_write(sc, n, APUREG_FREQ_LOBYTE, APU_plus6dB
		    | ((ch->dv & 0xff) << APU_FREQ_LOBYTE_SHIFT));
		wp_apu_write(sc, n, APUREG_FREQ_HIWORD, ch->dv >> 8);
		wc_ctrl_write(sc, n, wcreg);
		wc_ctrl_write(sc, n+1, wcreg);
		wp_apu_write(sc, n, APUREG_APUTYPE,
		    (aputype << APU_APUTYPE_SHIFT) | APU_DMA_ENABLED | 0xf);
		wp_apu_write(sc, n+1, APUREG_APUTYPE,
		    (aputype << APU_APUTYPE_SHIFT) | APU_DMA_ENABLED | 0xf);
	}
}

int
maestro_trigger_output(hdl, start, end, blksize, intr, arg, param)
	void *hdl;
	void *start, *end;
	int blksize;
	void (*intr) __P((void *));
	void *arg;
	struct audio_params *param;
{
	struct maestro_softc *sc = (struct maestro_softc *)hdl;

	u_int offset = ((caddr_t)start - sc->dmabase) >> 1;
	u_int size = (end - start) >> 1;
	sc->play.mode |= MAESTRO_RUNNING;
	sc->play.wpwa = APU_USE_SYSMEM | (offset >> 8);
	DPRINTF(("maestro_trigger_output: start=%x, end=%x, blksize=%x ",
		start, end, blksize));
    	DPRINTF(("offset = %x, size=%x\n", offset, size));

	sc->play.intr = intr;
	sc->play.intr_arg = arg;
	sc->play.blocksize = blksize;
	sc->play.end = offset+size;
	sc->play.start = offset;
	sc->play.current = sc->play.start;
	if ((sc->play.mode & (MAESTRO_STEREO | MAESTRO_8BIT)) == MAESTRO_STEREO) {
		sc->play.wpwa >>= 1;
		sc->play.start >>= 1;
		sc->play.end >>= 1;
		sc->play.blocksize >>= 1;
	}
	maestro_channel_start(&sc->play);

	sc->play.threshold = sc->play.start;
	maestro_update_timer(sc);

	return 0;
}

/* -----------------------------
 * Codec interface
 */

int
maestro_read_codec(self, regno, datap)
	void *self;
	u_int8_t regno;
	u_int16_t *datap;
{
	struct maestro_softc *sc = (struct maestro_softc *)self;
	int t;

	/* We have to wait for a SAFE time to write addr/data */
	for (t = 0; t < 20; t++) {
		if ((bus_space_read_1(sc->iot, sc->ioh, PORT_CODEC_STAT)
		    & CODEC_STAT_MASK) != CODEC_STAT_PROGLESS)
			break;
		DELAY(2);	/* 20.8us / 13 */
	}
	if (t == 20)
		printf("%s: maestro_read_codec() PROGLESS timed out.\n",
		    sc->dev.dv_xname);
		/* XXX return 1 */

	bus_space_write_1(sc->iot, sc->ioh, PORT_CODEC_CMD,
	    CODEC_CMD_READ | regno);
	DELAY(21);	/* AC97 cycle = 20.8usec */

	/* Wait for data retrieve */
	for (t = 0; t < 20; t++) {
		if ((bus_space_read_1(sc->iot, sc->ioh, PORT_CODEC_STAT)
		    & CODEC_STAT_MASK) == CODEC_STAT_RW_DONE)
			break;
		DELAY(2);	/* 20.8us / 13 */
	}
	if (t == 20)
		/* Timed out, but perform dummy read. */
		printf("%s: maestro_read_codec() RW_DONE timed out.\n",
		    sc->dev.dv_xname);

	*datap = bus_space_read_2(sc->iot, sc->ioh, PORT_CODEC_REG);
	return 0;
}

int
maestro_write_codec(self, regno, data)
	void *self;
	u_int8_t regno;
	u_int16_t data;
{
	struct maestro_softc *sc = (struct maestro_softc *)self;
	int t;

	/* We have to wait for a SAFE time to write addr/data */
	for (t = 0; t < 20; t++) {
		if ((bus_space_read_1(sc->iot, sc->ioh, PORT_CODEC_STAT)
		    & CODEC_STAT_MASK) != CODEC_STAT_PROGLESS)
			break;
		DELAY(2);	/* 20.8us / 13 */
	}
	if (t == 20) {
		/* Timed out. Abort writing. */
		printf("%s: maestro_write_codec() PROGLESS timed out.\n",
		    sc->dev.dv_xname);
		return 1;
	}

	bus_space_write_2(sc->iot, sc->ioh, PORT_CODEC_REG, data);
	bus_space_write_1(sc->iot, sc->ioh, PORT_CODEC_CMD,
	    CODEC_CMD_WRITE | regno);

	return 0;
}

int
maestro_attach_codec(self, cif)
	void *self;
	struct ac97_codec_if *cif;
{
	struct maestro_softc *sc = (struct maestro_softc *)self;

	sc->codec_if = cif;
	return 0;
}

void
maestro_reset_codec(self)
	void *self UNUSED;
{
}

void
maestro_initcodec(self)
	void *self;
{
	struct maestro_softc *sc = (struct maestro_softc *)self;
	u_int16_t data;

	if (bus_space_read_4(sc->iot, sc->ioh, PORT_RINGBUS_CTRL)
	    & RINGBUS_CTRL_ACLINK_ENABLED) {
		bus_space_write_4(sc->iot, sc->ioh, PORT_RINGBUS_CTRL, 0);
		DELAY(104);	/* 20.8us * (4 + 1) */
	}
	/* XXX - 2nd codec should be looked at. */
	bus_space_write_4(sc->iot, sc->ioh,
	    PORT_RINGBUS_CTRL, RINGBUS_CTRL_AC97_SWRESET);
	DELAY(2);
	bus_space_write_4(sc->iot, sc->ioh,
	    PORT_RINGBUS_CTRL, RINGBUS_CTRL_ACLINK_ENABLED);
	DELAY(21);

	maestro_read_codec(sc, 0, &data);
	if ((bus_space_read_1(sc->iot, sc->ioh, PORT_CODEC_STAT)
	    & CODEC_STAT_MASK) != 0) {
		bus_space_write_4(sc->iot, sc->ioh,
		    PORT_RINGBUS_CTRL, 0);
		DELAY(21);

		/* Try cold reset. */
		printf("%s: resetting codec\n", sc->dev.dv_xname);

		data = bus_space_read_2(sc->iot, sc->ioh, PORT_GPIO_DIR);
		if (pci_conf_read(sc->pc, sc->pt, 0x58) & 1)
			data |= 0x10;
		data |= 0x009 &
		    ~bus_space_read_2(sc->iot, sc->ioh, PORT_GPIO_DATA);
		bus_space_write_2(sc->iot, sc->ioh,
		    PORT_GPIO_MASK, 0xff6);
		bus_space_write_2(sc->iot, sc->ioh,
		    PORT_GPIO_DIR, data | 0x009);
		bus_space_write_2(sc->iot, sc->ioh,
		    PORT_GPIO_DATA, 0x000);
		DELAY(2);
		bus_space_write_2(sc->iot, sc->ioh,
		    PORT_GPIO_DATA, 0x001);
		DELAY(1);
		bus_space_write_2(sc->iot, sc->ioh,
		    PORT_GPIO_DATA, 0x009);
		DELAY(500000);
		bus_space_write_2(sc->iot, sc->ioh,
		    PORT_GPIO_DIR, data);
		DELAY(84);	/* 20.8us * 4 */
		bus_space_write_4(sc->iot, sc->ioh,
		    PORT_RINGBUS_CTRL, RINGBUS_CTRL_ACLINK_ENABLED);
		DELAY(21);
	}

	/* Check the codec to see is still busy */
	if ((bus_space_read_1(sc->iot, sc->ioh, PORT_CODEC_STAT) & 
	    CODEC_STAT_MASK) != 0) {
		printf("%s: codec failure\n", sc->dev.dv_xname);
	}
}

/* -----------------------------
 * Power management interface
 */

void
maestro_powerhook(why, self)
	int why;
	void *self;
{
	struct maestro_softc *sc = (struct maestro_softc *)self;

	if (why != PWR_RESUME) {
		/* Power down device on shutdown. */
		DPRINTF(("maestro: power down\n"));
		sc->suspend = why;
		if (sc->record.mode & MAESTRO_RUNNING) {
		    	sc->record.current = wp_apu_read(sc, sc->record.num, APUREG_CURPTR);
			maestro_channel_stop(&sc->record);
		}
		if (sc->play.mode & MAESTRO_RUNNING) {
		    	sc->play.current = wp_apu_read(sc, sc->play.num, APUREG_CURPTR);
			maestro_channel_stop(&sc->play);
		}

		wp_stoptimer(sc);

		/* Power down everything except clock. */
		bus_space_write_2(sc->iot, sc->ioh, PORT_HOSTINT_CTRL, 0);
		maestro_write_codec(sc, AC97_REG_POWER, 0xdf00);
		DELAY(20);
		bus_space_write_4(sc->iot, sc->ioh, PORT_RINGBUS_CTRL, 0);
		DELAY(1);
		maestro_power(sc, PPMI_D3);
	} else {
		/* Power up device on resume. */
		DPRINTF(("maestro: power resume\n"));
		if (sc->suspend == PWR_RESUME) {
			printf("%s: resume without suspend?\n");
			sc->suspend = why;
			return;
		}
		sc->suspend = why;
		maestro_power(sc, PPMI_D0);
		DELAY(100000);
		maestro_init(sc);
		/* Restore codec settings */
		if (sc->codec_if)
			sc->codec_if->vtbl->restore_ports(sc->codec_if);
		if (sc->play.mode & MAESTRO_RUNNING)
			maestro_channel_start(&sc->play);
		if (sc->record.mode & MAESTRO_RUNNING)
			maestro_channel_start(&sc->record);
		maestro_update_timer(sc);
	}
}

void
maestro_power(sc, status)
	struct maestro_softc *sc;
	int status;
{
	int data;

	/* Set the power state of the device. */
	data = pci_conf_read(sc->pc, sc->pt, CONF_PM_PTR);
	data = pci_conf_read(sc->pc, sc->pt, data);
	if (data == PPMI_CID)
		pci_conf_write(sc->pc, sc->pt, data + PM_CTRL, status);
}

void
maestro_channel_advance_dma(ch)
	struct maestro_channel *ch;
{
	wpreg_t pos;
#ifdef AUDIO_DEBUG
	maestrointr_called++;
#endif
	for (;;) {
		pos = wp_apu_read(ch->sc, ch->num, APUREG_CURPTR);
		/* Are we still processing the current dma block ? */
		if (pos >= ch->threshold && 
		    pos < ch->threshold + ch->blocksize/2)
			break;
		ch->threshold += ch->blocksize/2;
		if (ch->threshold >= ch->end)
			ch->threshold = ch->start;
		(*ch->intr)(ch->intr_arg);
#ifdef AUDIO_DEBUG
		maestrodma_effective++;
#endif
	}

#ifdef AUDIO_DEBUG
	if (maestrodebug && maestrointr_called % 64 == 0)
		printf("maestro: dma advanced %lu for %lu calls\n", 
			maestrodma_effective, maestrointr_called);
#endif
}

/* -----------------------------
 * Interrupt handler interface
 */
int
maestro_intr(arg)
	void *arg;
{
	struct maestro_softc *sc = (struct maestro_softc *)arg;
	u_int16_t status;

	status = bus_space_read_1(sc->iot, sc->ioh, PORT_HOSTINT_STAT);
	if (status == 0)
		return 0;	/* Not for us? */

	/* Acknowledge all. */
	bus_space_write_2(sc->iot, sc->ioh, PORT_INT_STAT, 1);
	bus_space_write_1(sc->iot, sc->ioh, PORT_HOSTINT_STAT, status);

	/* Hardware volume support */
	if (status & HOSTINT_STAT_HWVOL && sc->codec_if != NULL) {
		int n, i, delta, v;
		mixer_ctrl_t hwvol;

		n = bus_space_read_1(sc->iot, sc->ioh, PORT_HWVOL_MASTER);
		/* Special case: Mute key */
		if (n & 0x11) {
			hwvol.type = AUDIO_MIXER_ENUM;
			hwvol.dev = 
			    sc->codec_if->vtbl->get_portnum_by_name(sc->codec_if,
				AudioCoutputs, AudioNmaster, AudioNmute);
			sc->codec_if->vtbl->mixer_get_port(sc->codec_if, &hwvol);
			hwvol.un.ord = !hwvol.un.ord;
		} else {
			hwvol.type = AUDIO_MIXER_VALUE;
			hwvol.un.value.num_channels = 2;
			hwvol.dev = 
			    sc->codec_if->vtbl->get_portnum_by_name(
			    	sc->codec_if, AudioCoutputs, AudioNmaster, 
				    NULL);
			sc->codec_if->vtbl->mixer_get_port(sc->codec_if, &hwvol);
			/* XXX AC'97 yields five bits for master volume. */
			delta = (n - MIDDLE_VOLUME)/STEP_VOLUME * 8;
			for (i = 0; i < hwvol.un.value.num_channels; i++) {
				v = ((int)hwvol.un.value.level[i]) + delta;
				if (v < 0)
					v = 0;
				else if (v > 255)
					v = 255;
				hwvol.un.value.level[i] = v;
			}
		}
		sc->codec_if->vtbl->mixer_set_port(sc->codec_if, &hwvol);
		/* Reset to compute next diffs */
		bus_space_write_1(sc->iot, sc->ioh, PORT_HWVOL_MASTER, 
		    MIDDLE_VOLUME);
	}

	if (sc->play.mode & MAESTRO_RUNNING)
		maestro_channel_advance_dma(&sc->play);
	/* XXX suppress jitter for stereo play? */

	if (sc->record.mode & MAESTRO_RUNNING)
		maestro_channel_advance_dma(&sc->record);

	return 1;
}

/* -----------------------------
 * Hardware interface
 */

/* Codec/Ringbus */

void
ringbus_setdest(struct maestro_softc *sc, int src, int dest)
{
	u_int32_t	data;

	data = bus_space_read_4(sc->iot, sc->ioh, PORT_RINGBUS_CTRL);
	data &= ~(0xfU << src);
	data |= (0xfU & dest) << src;
	bus_space_write_4(sc->iot, sc->ioh, PORT_RINGBUS_CTRL, data);
}

/* Wave Processor */

wpreg_t
wp_reg_read(struct maestro_softc *sc, int reg)
{
	bus_space_write_2(sc->iot, sc->ioh, PORT_DSP_INDEX, reg);
	return bus_space_read_2(sc->iot, sc->ioh, PORT_DSP_DATA);
}

void
wp_reg_write(struct maestro_softc *sc, int reg, wpreg_t data)
{
	bus_space_write_2(sc->iot, sc->ioh, PORT_DSP_INDEX, reg);
	bus_space_write_2(sc->iot, sc->ioh, PORT_DSP_DATA, data);
}

static void
apu_setindex(struct maestro_softc *sc, int reg)
{
	int t;

	wp_reg_write(sc, WPREG_CRAM_PTR, reg);
	/* Sometimes WP fails to set apu register index. */
	for (t = 0; t < 1000; t++) {
		if (bus_space_read_2(sc->iot, sc->ioh,
		    PORT_DSP_DATA) == reg)
			break;
		bus_space_write_2(sc->iot, sc->ioh, PORT_DSP_DATA, reg);
	}
	if (t == 1000)
		printf("%s: apu_setindex() timeout\n", sc->dev.dv_xname);
}

wpreg_t
wp_apu_read(struct maestro_softc *sc, int ch, int reg)
{
	wpreg_t ret;

	apu_setindex(sc, ((unsigned)ch << 4) + reg);
	ret = wp_reg_read(sc, WPREG_DATA_PORT);
	return ret;
}

void
wp_apu_write(struct maestro_softc *sc, int ch, int reg, wpreg_t data)
{
	int t;

	apu_setindex(sc, ((unsigned)ch << 4) + reg);
	wp_reg_write(sc, WPREG_DATA_PORT, data);
	for (t = 0; t < 1000; t++) {
		if (bus_space_read_2(sc->iot, sc->ioh, PORT_DSP_DATA) == data)
			break;
		bus_space_write_2(sc->iot, sc->ioh, PORT_DSP_DATA, data);
	}
	if (t == 1000)
		printf("%s: wp_apu_write() timeout\n", sc->dev.dv_xname);
}

void
wp_settimer(struct maestro_softc *sc, u_int freq)
{
	u_int clock = 48000 << 2;
	u_int prescale = 0, divide = (freq != 0) ? (clock / freq) : ~0;

	if (divide < 4)
		divide = 4;
	else if (divide > 32 << 8)
		divide = 32 << 8;

	for (; divide > 32 << 1; divide >>= 1)
		prescale++;
	divide = (divide + 1) >> 1;

	for (; prescale < 7 && divide > 2 && !(divide & 1); divide >>= 1)
		prescale++;

	wp_reg_write(sc, WPREG_TIMER_ENABLE, 0);
	wp_reg_write(sc, WPREG_TIMER_FREQ,
	    (prescale << WP_TIMER_FREQ_PRESCALE_SHIFT) | (divide - 1));
	wp_reg_write(sc, WPREG_TIMER_ENABLE, 1);
}

void
wp_starttimer(struct maestro_softc *sc)
{
	wp_reg_write(sc, WPREG_TIMER_START, 1);
}

void
wp_stoptimer(struct maestro_softc *sc)
{
	wp_reg_write(sc, WPREG_TIMER_START, 0);
	bus_space_write_2(sc->iot, sc->ioh, PORT_INT_STAT, 1);
}

/* WaveCache */

wcreg_t
wc_reg_read(struct maestro_softc *sc, int reg)
{
	bus_space_write_2(sc->iot, sc->ioh, PORT_WAVCACHE_INDEX, reg);
	return bus_space_read_2(sc->iot, sc->ioh, PORT_WAVCACHE_DATA);
}

void
wc_reg_write(struct maestro_softc *sc, int reg, wcreg_t data)
{
	bus_space_write_2(sc->iot, sc->ioh, PORT_WAVCACHE_INDEX, reg);
	bus_space_write_2(sc->iot, sc->ioh, PORT_WAVCACHE_DATA, data);
}

u_int16_t
wc_ctrl_read(struct maestro_softc *sc, int ch)
{
	return wc_reg_read(sc, ch << 3);
}

void
wc_ctrl_write(struct maestro_softc *sc, int ch, wcreg_t data)
{
	wc_reg_write(sc, ch << 3, data);
}

/* -----------------------------
 * Simple zone allocator.
 * (All memory allocated in advance)
 */

salloc_t
salloc_new(addr, size, nzones)
	caddr_t addr;
	size_t size;
	int nzones;
{
	struct salloc_pool *pool;
	struct salloc_zone *space;
	int i;

	MALLOC(pool, salloc_t, sizeof *pool + nzones * sizeof pool->zones[0],
	    M_TEMP, M_WAITOK);
	if (pool == NULL)
		return NULL;
	SLIST_INIT(&pool->free);
	SLIST_INIT(&pool->used);
	SLIST_INIT(&pool->spare);
	/* Espie says the following line is obvious */
	pool->zones = (struct salloc_zone *)(pool + 1);
	for (i = 1; i < nzones; i++)
		SLIST_INSERT_HEAD(&pool->spare, &pool->zones[i], link);
	space = &pool->zones[0];
	space->addr = addr;
	space->size = size;
	SLIST_INSERT_HEAD(&pool->free, space, link);
	return pool;
}

void
salloc_destroy(pool)
	salloc_t pool;
{
	FREE(pool, M_TEMP);
}

void
salloc_insert(pool, head, zone, merge)
	salloc_t pool;
	struct salloc_head *head;
	struct salloc_zone *zone;
	int merge;
{
	struct salloc_zone *prev, *next;

	/* 
	 * Insert a zone into an ordered list of zones, possibly
	 * merging adjacent zones.
	 */
	prev = NULL;
	SLIST_FOREACH(next, head, link) {
		if (next->addr > zone->addr) 
			break;
		prev = next;
	}

	if (merge && prev && prev->addr + prev->size == zone->addr) {
		prev->size += zone->size;
		SLIST_INSERT_HEAD(&pool->spare, zone, link);
		zone = prev;
	} else if (prev)
		SLIST_INSERT_AFTER(prev, zone, link);
	else
		SLIST_INSERT_HEAD(head, zone, link);
	if (merge && next && zone->addr + zone->size == next->addr) {
		zone->size += next->size;
		SLIST_REMOVE(head, next, salloc_zone, link);
		SLIST_INSERT_HEAD(&pool->spare, next, link);
	}
}

caddr_t
salloc_alloc(pool, size)
	salloc_t pool;
	size_t size;
{
	struct salloc_zone *zone, *uzone;

	SLIST_FOREACH(zone, &pool->free, link) 
		if (zone->size >= size)
			break;
	if (zone == SLIST_END(&pool->free))
		return NULL;
	if (zone->size == size) {
		SLIST_REMOVE(&pool->free, zone, salloc_zone, link);
		uzone = zone;
	} else {
		uzone = SLIST_FIRST(&pool->spare);
		if (uzone == NULL)
			return NULL;		/* XXX */
		SLIST_REMOVE_HEAD(&pool->spare, link);
		uzone->size = size;
		uzone->addr = zone->addr;
		zone->size -= size;
		zone->addr += size;
	}
	salloc_insert(pool, &pool->used, uzone, 0);
	return uzone->addr;
}

void
salloc_free(pool, addr)
	salloc_t pool;
	caddr_t addr;
{
	struct salloc_zone *zone;

	SLIST_FOREACH(zone, &pool->used, link) 
		if (zone->addr == addr)
			break;
#ifdef DIAGNOSTIC
	if (zone == SLIST_END(&pool->used))
		panic("salloc_free: freeing unallocated memory");
#endif
	SLIST_REMOVE(&pool->used, zone, salloc_zone, link);
	salloc_insert(pool, &pool->free, zone, 1);
}
