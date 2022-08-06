// SPDX-License-Identifier: GPL-2.0
/*
 * Synopsys DesignWare Cores DisplayPort Transmitter Controller
 *
 * Copyright (c) 2021 Rockchip Electronics Co. Ltd.
 *
 * Author: Wyon Bi <bivvy.bi@rock-chips.com>
 *	   Zhang Yubing <yubing.zhang@rock-chips.com>
 */

#include <asm/unaligned.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_bridge.h>
#include <drm/drm_dp_helper.h>
#include <drm/drm_of.h>
#include <drm/drm_print.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_simple_kms_helper.h>

#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/component.h>
#include <linux/extcon-provider.h>
#include <linux/iopoll.h>
#include <linux/irq.h>
#include <linux/of_device.h>
#include <linux/of_graph.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/gpio/consumer.h>
#include <linux/phy/phy.h>
#include <linux/mfd/syscon.h>

#include <sound/hdmi-codec.h>

#include <uapi/linux/videodev2.h>

#include "rockchip_drm_drv.h"
#include "rockchip_drm_vop.h"

#define DPTX_VERSION_NUMBER			0x0000
#define DPTX_VERSION_TYPE			0x0004
#define DPTX_ID					0x0008

#define DPTX_CONFIG_REG1			0x0100
#define DPTX_CONFIG_REG2			0x0104
#define DPTX_CONFIG_REG3			0x0108

#define DPTX_CCTL				0x0200
#define FORCE_HPD				BIT(4)
#define DEFAULT_FAST_LINK_TRAIN_EN		BIT(2)
#define ENHANCE_FRAMING_EN			BIT(1)
#define SCRAMBLE_DIS				BIT(0)
#define DPTX_SOFT_RESET_CTRL			0x0204
#define VIDEO_RESET				BIT(5)
#define AUX_RESET				BIT(4)
#define AUDIO_SAMPLER_RESET			BIT(3)
#define PHY_SOFT_RESET				BIT(1)
#define CONTROLLER_RESET			BIT(0)

#define DPTX_VSAMPLE_CTRL			0x0300
#define PIXEL_MODE_SELECT			GENMASK(22, 21)
#define VIDEO_MAPPING				GENMASK(20, 16)
#define VIDEO_STREAM_ENABLE			BIT(5)
#define DPTX_VSAMPLE_STUFF_CTRL1		0x0304
#define DPTX_VSAMPLE_STUFF_CTRL2		0x0308
#define DPTX_VINPUT_POLARITY_CTRL		0x030c
#define DE_IN_POLARITY				BIT(2)
#define HSYNC_IN_POLARITY			BIT(1)
#define VSYNC_IN_POLARITY			BIT(0)
#define DPTX_VIDEO_CONFIG1			0x0310
#define HACTIVE					GENMASK(31, 16)
#define HBLANK					GENMASK(15, 2)
#define I_P					BIT(1)
#define R_V_BLANK_IN_OSC			BIT(0)
#define DPTX_VIDEO_CONFIG2			0x0314
#define VBLANK					GENMASK(31, 16)
#define VACTIVE					GENMASK(15, 0)
#define DPTX_VIDEO_CONFIG3			0x0318
#define H_SYNC_WIDTH				GENMASK(31, 16)
#define H_FRONT_PORCH				GENMASK(15, 0)
#define DPTX_VIDEO_CONFIG4			0x031c
#define V_SYNC_WIDTH				GENMASK(31, 16)
#define V_FRONT_PORCH				GENMASK(15, 0)
#define DPTX_VIDEO_CONFIG5			0x0320
#define INIT_THRESHOLD_HI			GENMASK(22, 21)
#define AVERAGE_BYTES_PER_TU_FRAC		GENMASK(19, 16)
#define INIT_THRESHOLD				GENMASK(13, 7)
#define AVERAGE_BYTES_PER_TU			GENMASK(6, 0)
#define DPTX_VIDEO_MSA1				0x0324
#define VSTART					GENMASK(31, 16)
#define HSTART					GENMASK(15, 0)
#define DPTX_VIDEO_MSA2				0x0328
#define MISC0					GENMASK(31, 24)
#define DPTX_VIDEO_MSA3				0x032c
#define MISC1					GENMASK(31, 24)
#define DPTX_VIDEO_HBLANK_INTERVAL		0x0330
#define HBLANK_INTERVAL_EN			BIT(16)
#define HBLANK_INTERVAL				GENMASK(15, 0)

#define DPTX_AUD_CONFIG1			0x0400
#define AUDIO_TIMESTAMP_VERSION_NUM		GENMASK(29, 24)
#define AUDIO_PACKET_ID				GENMASK(23, 16)
#define AUDIO_MUTE				BIT(15)
#define NUM_CHANNELS				GENMASK(14, 12)
#define HBR_MODE_ENABLE				BIT(10)
#define AUDIO_DATA_WIDTH			GENMASK(9, 5)
#define AUDIO_DATA_IN_EN			GENMASK(4, 1)
#define AUDIO_INF_SELECT			BIT(0)

#define DPTX_SDP_VERTICAL_CTRL			0x0500
#define EN_VERTICAL_SDP				BIT(2)
#define EN_AUDIO_STREAM_SDP			BIT(1)
#define EN_AUDIO_TIMESTAMP_SDP			BIT(0)
#define DPTX_SDP_HORIZONTAL_CTRL		0x0504
#define EN_HORIZONTAL_SDP			BIT(2)
#define DPTX_SDP_STATUS_REGISTER		0x0508
#define DPTX_SDP_MANUAL_CTRL			0x050c
#define DPTX_SDP_STATUS_EN			0x0510

#define DPTX_SDP_REGISTER_BANK			0x0600
#define SDP_REGS				GENMASK(31, 0)

#define DPTX_PHYIF_CTRL				0x0a00
#define PHY_WIDTH				BIT(25)
#define PHY_POWERDOWN				GENMASK(20, 17)
#define PHY_BUSY				GENMASK(15, 12)
#define SSC_DIS					BIT(16)
#define XMIT_ENABLE				GENMASK(11, 8)
#define PHY_LANES				GENMASK(7, 6)
#define PHY_RATE				GENMASK(5, 4)
#define TPS_SEL					GENMASK(3, 0)
#define DPTX_PHY_TX_EQ				0x0a04
#define DPTX_CUSTOMPAT0				0x0a08
#define DPTX_CUSTOMPAT1				0x0a0c
#define DPTX_CUSTOMPAT2				0x0a10
#define DPTX_HBR2_COMPLIANCE_SCRAMBLER_RESET	0x0a14
#define DPTX_PHYIF_PWRDOWN_CTRL			0x0a18

#define DPTX_AUX_CMD				0x0b00
#define AUX_CMD_TYPE				GENMASK(31, 28)
#define AUX_ADDR				GENMASK(27, 8)
#define I2C_ADDR_ONLY				BIT(4)
#define AUX_LEN_REQ				GENMASK(3, 0)
#define DPTX_AUX_STATUS				0x0b04
#define AUX_TIMEOUT				BIT(17)
#define AUX_BYTES_READ				GENMASK(23, 19)
#define AUX_STATUS				GENMASK(7, 4)
#define DPTX_AUX_DATA0				0x0b08
#define DPTX_AUX_DATA1				0x0b0c
#define DPTX_AUX_DATA2				0x0b10
#define DPTX_AUX_DATA3				0x0b14

#define DPTX_GENERAL_INTERRUPT			0x0d00
#define VIDEO_FIFO_OVERFLOW_STREAM0		BIT(6)
#define AUDIO_FIFO_OVERFLOW_STREAM0		BIT(5)
#define SDP_EVENT_STREAM0			BIT(4)
#define AUX_CMD_INVALID				BIT(3)
#define AUX_REPLY_EVENT				BIT(1)
#define HPD_EVENT				BIT(0)
#define DPTX_GENERAL_INTERRUPT_ENABLE		0x0d04
#define AUX_REPLY_EVENT_EN			BIT(1)
#define HPD_EVENT_EN				BIT(0)
#define DPTX_HPD_STATUS				0x0d08
#define HPD_STATE				GENMASK(11, 9)
#define HPD_STATUS				BIT(8)
#define HPD_HOT_UNPLUG				BIT(2)
#define HPD_HOT_PLUG				BIT(1)
#define HPD_IRQ					BIT(0)
#define DPTX_HPD_INTERRUPT_ENABLE		0x0d0c
#define HPD_UNPLUG_ERR_EN			BIT(3)
#define HPD_UNPLUG_EN				BIT(2)
#define HPD_PLUG_EN				BIT(1)
#define HPD_IRQ_EN				BIT(0)

#define DPTX_MAX_REGISTER			DPTX_HPD_INTERRUPT_ENABLE

#define SDP_REG_BANK_SIZE			16

struct drm_dp_link_caps {
	bool enhanced_framing;
	bool tps3_supported;
	bool tps4_supported;
	bool fast_training;
	bool channel_coding;
	bool ssc;
};

struct drm_dp_link_train_set {
	unsigned int voltage_swing[4];
	unsigned int pre_emphasis[4];
};

struct drm_dp_link_train {
	struct drm_dp_link_train_set request;
	struct drm_dp_link_train_set adjust;
	bool clock_recovered;
	bool channel_equalized;
};

struct dw_dp_link {
	u8 dpcd[DP_RECEIVER_CAP_SIZE];
	unsigned char revision;
	unsigned int rate;
	unsigned int lanes;
	struct drm_dp_link_caps caps;
	struct drm_dp_link_train train;
	struct drm_dp_desc desc;
	u8 sink_count;
	u8 vsc_sdp_extension_for_colorimetry_supported;
};

struct dw_dp_video {
	struct drm_display_mode mode;
	u32 bus_format;
	u8 video_mapping;
	u8 pixel_mode;
	u8 color_format;
	u8 bpc;
	u8 bpp;
};

struct dw_dp_audio {
	struct platform_device *pdev;
	u8 channels;
};

struct dw_dp_sdp {
	struct dp_sdp_header header;
	u8 db[32];
	unsigned long flags;
};

struct dw_dp_hotplug {
	bool long_hpd;
	bool status;
};

struct dw_dp {
	struct device *dev;
	struct regmap *regmap;
	struct phy *phy;
	struct clk_bulk_data *clks;
	int nr_clks;
	struct reset_control *rstc;
	struct regmap *grf;
	struct completion complete;
	int irq;
	int id;
	struct work_struct hpd_work;
	struct gpio_desc *hpd_gpio;
	struct dw_dp_hotplug hotplug;
	struct mutex irq_lock;
	struct extcon_dev *extcon;

	struct drm_bridge bridge;
	struct drm_connector connector;
	struct drm_encoder encoder;
	struct drm_dp_aux aux;

	struct dw_dp_link link;
	struct dw_dp_video video;
	struct dw_dp_audio audio;

	DECLARE_BITMAP(sdp_reg_bank, SDP_REG_BANK_SIZE);

	bool split_mode;
	struct dw_dp *left;
	struct dw_dp *right;
};

enum {
	DPTX_VM_RGB_6BIT,
	DPTX_VM_RGB_8BIT,
	DPTX_VM_RGB_10BIT,
	DPTX_VM_RGB_12BIT,
	DPTX_VM_RGB_16BIT,
	DPTX_VM_YCBCR444_8BIT,
	DPTX_VM_YCBCR444_10BIT,
	DPTX_VM_YCBCR444_12BIT,
	DPTX_VM_YCBCR444_16BIT,
	DPTX_VM_YCBCR422_8BIT,
	DPTX_VM_YCBCR422_10BIT,
	DPTX_VM_YCBCR422_12BIT,
	DPTX_VM_YCBCR422_16BIT,
	DPTX_VM_YCBCR420_8BIT,
	DPTX_VM_YCBCR420_10BIT,
	DPTX_VM_YCBCR420_12BIT,
	DPTX_VM_YCBCR420_16BIT,
};

enum {
	DPTX_MP_SINGLE_PIXEL,
	DPTX_MP_DUAL_PIXEL,
	DPTX_MP_QUAD_PIXEL,
};

enum {
	DPTX_SDP_VERTICAL_INTERVAL = BIT(0),
	DPTX_SDP_HORIZONTAL_INTERVAL = BIT(1),
};

enum {
	SOURCE_STATE_IDLE,
	SOURCE_STATE_UNPLUG,
	SOURCE_STATE_HPD_TIMEOUT = 4,
	SOURCE_STATE_PLUG = 7
};

enum {
	DPTX_PHY_PATTERN_NONE,
	DPTX_PHY_PATTERN_TPS_1,
	DPTX_PHY_PATTERN_TPS_2,
	DPTX_PHY_PATTERN_TPS_3,
	DPTX_PHY_PATTERN_TPS_4,
	DPTX_PHY_PATTERN_SERM,
	DPTX_PHY_PATTERN_PBRS7,
	DPTX_PHY_PATTERN_CUSTOM_80BIT,
	DPTX_PHY_PATTERN_CP2520_1,
	DPTX_PHY_PATTERN_CP2520_2,
};

static const unsigned int dw_dp_cable[] = {
	EXTCON_DISP_DP,
	EXTCON_NONE,
};

struct dw_dp_output_format {
	u32 bus_format;
	u32 color_format;
	u8 video_mapping;
	u8 bpc;
	u8 bpp;
};

static const struct dw_dp_output_format possible_output_fmts[] = {
	{ MEDIA_BUS_FMT_RGB101010_1X30, DRM_COLOR_FORMAT_RGB444,
	  DPTX_VM_RGB_10BIT, 10, 30 },
	{ MEDIA_BUS_FMT_RGB888_1X24, DRM_COLOR_FORMAT_RGB444,
	  DPTX_VM_RGB_8BIT, 8, 24 },
	{ MEDIA_BUS_FMT_YUV10_1X30, DRM_COLOR_FORMAT_YCRCB444,
	  DPTX_VM_YCBCR444_10BIT, 10, 30 },
	{ MEDIA_BUS_FMT_YUV8_1X24, DRM_COLOR_FORMAT_YCRCB444,
	  DPTX_VM_YCBCR444_8BIT, 8, 24},
	{ MEDIA_BUS_FMT_YUYV10_1X20, DRM_COLOR_FORMAT_YCRCB422,
	  DPTX_VM_YCBCR422_10BIT, 10, 20 },
	{ MEDIA_BUS_FMT_YUYV8_1X16, DRM_COLOR_FORMAT_YCRCB422,
	  DPTX_VM_YCBCR422_8BIT, 8, 16 },
	{ MEDIA_BUS_FMT_UYYVYY10_0_5X30, DRM_COLOR_FORMAT_YCRCB420,
	  DPTX_VM_YCBCR420_10BIT, 10, 15 },
	{ MEDIA_BUS_FMT_UYYVYY8_0_5X24, DRM_COLOR_FORMAT_YCRCB420,
	  DPTX_VM_YCBCR420_8BIT, 8, 12 },
	{ MEDIA_BUS_FMT_RGB666_1X24_CPADHI, DRM_COLOR_FORMAT_RGB444,
	  DPTX_VM_RGB_6BIT, 6, 18 },
};

static const struct dw_dp_output_format *dw_dp_get_output_format(u32 bus_format)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(possible_output_fmts); i++)
		if (possible_output_fmts[i].bus_format == bus_format)
			return &possible_output_fmts[i];

	return &possible_output_fmts[1];
}

static inline struct dw_dp *connector_to_dp(struct drm_connector *c)
{
	return container_of(c, struct dw_dp, connector);
}

static inline struct dw_dp *encoder_to_dp(struct drm_encoder *e)
{
	return container_of(e, struct dw_dp, encoder);
}

static inline struct dw_dp *bridge_to_dp(struct drm_bridge *b)
{
	return container_of(b, struct dw_dp, bridge);
}

static int dw_dp_match_by_id(struct device *dev, const void *data)
{
	struct dw_dp *dp = dev_get_drvdata(dev);
	const unsigned int *id = data;

	return dp->id == *id;
}

static struct dw_dp *dw_dp_find_by_id(struct device_driver *drv,
				      unsigned int id)
{
	struct device *dev;

	dev = driver_find_device(drv, NULL, &id, dw_dp_match_by_id);
	if (!dev)
		return NULL;

	return dev_get_drvdata(dev);
}

static void dw_dp_phy_set_pattern(struct dw_dp *dp, u32 pattern)
{
	regmap_update_bits(dp->regmap, DPTX_PHYIF_CTRL, TPS_SEL,
			   FIELD_PREP(TPS_SEL, pattern));
}

static void dw_dp_phy_xmit_enable(struct dw_dp *dp, u32 lanes)
{
	u32 xmit_enable;

	switch (lanes) {
	case 4:
	case 2:
	case 1:
		xmit_enable = GENMASK(lanes - 1, 0);
		break;
	case 0:
	default:
		xmit_enable = 0;
		break;
	}

	regmap_update_bits(dp->regmap, DPTX_PHYIF_CTRL, XMIT_ENABLE,
			   FIELD_PREP(XMIT_ENABLE, xmit_enable));
}

static bool dw_dp_bandwidth_ok(struct dw_dp *dp,
			       const struct drm_display_mode *mode, u32 bpp,
			       unsigned int lanes, unsigned int rate)
{
	u32 max_bw, req_bw;

	req_bw = mode->clock * bpp / 8;
	max_bw = lanes * rate;
	if (req_bw > max_bw)
		return false;

	return true;
}

static bool dw_dp_detect(struct dw_dp *dp)
{
	u32 value;

	if (dp->hpd_gpio)
		return gpiod_get_value_cansleep(dp->hpd_gpio);

	regmap_read(dp->regmap, DPTX_HPD_STATUS, &value);

	return FIELD_GET(HPD_STATE, value) == SOURCE_STATE_PLUG;
}

static enum drm_connector_status
dw_dp_connector_detect(struct drm_connector *connector, bool force)
{
	struct dw_dp *dp = connector_to_dp(connector);

	if (dp->right && drm_bridge_detect(&dp->right->bridge) != connector_status_connected)
		return connector_status_disconnected;

	return drm_bridge_detect(&dp->bridge);
}

static void dw_dp_connector_force(struct drm_connector *connector)
{
	struct dw_dp *dp = connector_to_dp(connector);

	if (connector->status == connector_status_connected)
		extcon_set_state_sync(dp->extcon, EXTCON_DISP_DP, true);
	else
		extcon_set_state_sync(dp->extcon, EXTCON_DISP_DP, false);
}

static const struct drm_connector_funcs dw_dp_connector_funcs = {
	.detect			= dw_dp_connector_detect,
	.fill_modes		= drm_helper_probe_single_connector_modes,
	.destroy		= drm_connector_cleanup,
	.force			= dw_dp_connector_force,
	.reset			= drm_atomic_helper_connector_reset,
	.atomic_duplicate_state	= drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_connector_destroy_state,
};

static int dw_dp_connector_get_modes(struct drm_connector *connector)
{
	struct dw_dp *dp = connector_to_dp(connector);
	struct drm_display_info *di = &connector->display_info;
	struct edid *edid;
	int num_modes = 0;

	edid = drm_bridge_get_edid(&dp->bridge, connector);
	if (edid) {
		drm_connector_update_edid_property(connector, edid);
		num_modes = drm_add_edid_modes(connector, edid);
		kfree(edid);
	}

	if (!di->color_formats)
		di->color_formats = DRM_COLOR_FORMAT_RGB444;

	if (!di->bpc)
		di->bpc = 8;

	if (num_modes > 0 && dp->split_mode) {
		struct drm_display_mode *mode;

		di->width_mm *= 2;

		list_for_each_entry(mode, &connector->probed_modes, head)
			drm_mode_convert_to_split_mode(mode);
	}

	return num_modes;
}

static const struct drm_connector_helper_funcs dw_dp_connector_helper_funcs = {
	.get_modes = dw_dp_connector_get_modes,
};

static void dw_dp_link_caps_reset(struct drm_dp_link_caps *caps)
{
	caps->enhanced_framing = false;
	caps->tps3_supported = false;
	caps->tps4_supported = false;
	caps->fast_training = false;
	caps->channel_coding = false;
}

static void dw_dp_link_reset(struct dw_dp_link *link)
{
	link->vsc_sdp_extension_for_colorimetry_supported = 0;
	link->sink_count = 0;
	link->revision = 0;

	dw_dp_link_caps_reset(&link->caps);
	memset(link->dpcd, 0, sizeof(link->dpcd));

	link->rate = 0;
	link->lanes = 0;
}

static int dw_dp_link_power_up(struct dw_dp *dp)
{
	struct dw_dp_link *link = &dp->link;
	u8 value;
	int ret;

	if (link->revision < 0x11)
		return 0;

	ret = drm_dp_dpcd_readb(&dp->aux, DP_SET_POWER, &value);
	if (ret < 0)
		return ret;

	value &= ~DP_SET_POWER_MASK;
	value |= DP_SET_POWER_D0;

	ret = drm_dp_dpcd_writeb(&dp->aux, DP_SET_POWER, value);
	if (ret < 0)
		return ret;

	usleep_range(1000, 2000);

	return 0;
}

static int dw_dp_link_power_down(struct dw_dp *dp)
{
	struct dw_dp_link *link = &dp->link;
	u8 value;
	int ret;

	if (link->revision < 0x11)
		return 0;

	ret = drm_dp_dpcd_readb(&dp->aux, DP_SET_POWER, &value);
	if (ret < 0)
		return ret;

	value &= ~DP_SET_POWER_MASK;
	value |= DP_SET_POWER_D3;

	ret = drm_dp_dpcd_writeb(&dp->aux, DP_SET_POWER, value);
	if (ret < 0)
		return ret;

	return 0;
}

static bool dw_dp_has_sink_count(const u8 dpcd[DP_RECEIVER_CAP_SIZE],
				 const struct drm_dp_desc *desc)
{
	return dpcd[DP_DPCD_REV] >= DP_DPCD_REV_11 &&
	       dpcd[DP_DOWNSTREAMPORT_PRESENT] & DP_DWN_STRM_PORT_PRESENT &&
	       !drm_dp_has_quirk(desc, 0, DP_DPCD_QUIRK_NO_SINK_COUNT);
}

static int dw_dp_link_probe(struct dw_dp *dp)
{
	struct dw_dp_link *link = &dp->link;
	u8 dpcd;
	int ret;

	dw_dp_link_reset(link);

	ret = drm_dp_read_dpcd_caps(&dp->aux, link->dpcd);
	if (ret < 0)
		return ret;

	drm_dp_read_desc(&dp->aux, &link->desc, drm_dp_is_branch(link->dpcd));

	if (dw_dp_has_sink_count(link->dpcd, &link->desc)) {
		ret = drm_dp_read_sink_count(&dp->aux);
		if (ret < 0)
			return ret;

		link->sink_count = ret;

		/* Dongle connected, but no display */
		if (!link->sink_count)
			return -ENODEV;
	}

	ret = drm_dp_dpcd_readb(&dp->aux, DP_DPRX_FEATURE_ENUMERATION_LIST,
				&dpcd);
	if (ret < 0)
		return ret;

	link->vsc_sdp_extension_for_colorimetry_supported =
			!!(dpcd & DP_VSC_SDP_EXT_FOR_COLORIMETRY_SUPPORTED);

	link->revision = link->dpcd[DP_DPCD_REV];
	link->rate = drm_dp_max_link_rate(link->dpcd);
	link->lanes = min_t(u8, phy_get_bus_width(dp->phy),
			    drm_dp_max_lane_count(link->dpcd));

	link->caps.enhanced_framing = drm_dp_enhanced_frame_cap(link->dpcd);
	link->caps.tps3_supported = drm_dp_tps3_supported(link->dpcd);
	link->caps.tps4_supported = drm_dp_tps4_supported(link->dpcd);
	link->caps.fast_training = drm_dp_fast_training_cap(link->dpcd);
	link->caps.channel_coding = drm_dp_channel_coding_supported(link->dpcd);
	link->caps.ssc = !!(link->dpcd[DP_MAX_DOWNSPREAD] & DP_MAX_DOWNSPREAD_0_5);

	return 0;
}

static int dw_dp_link_train_update_vs_emph(struct dw_dp *dp)
{
	struct dw_dp_link *link = &dp->link;
	struct drm_dp_link_train_set *request = &link->train.request;
	union phy_configure_opts phy_cfg;
	unsigned int lanes = link->lanes, *vs, *pe;
	u8 buf[4];
	int i, ret;

	vs = request->voltage_swing;
	pe = request->pre_emphasis;

	for (i = 0; i < lanes; i++) {
		phy_cfg.dp.voltage[i] = vs[i];
		phy_cfg.dp.pre[i] = pe[i];
	}
	phy_cfg.dp.lanes = lanes;
	phy_cfg.dp.link_rate = link->rate / 100;
	phy_cfg.dp.set_lanes = false;
	phy_cfg.dp.set_rate = false;
	phy_cfg.dp.set_voltages = true;
	ret = phy_configure(dp->phy, &phy_cfg);
	if (ret)
		return ret;

	for (i = 0; i < lanes; i++)
		buf[i] = (vs[i] << DP_TRAIN_VOLTAGE_SWING_SHIFT) |
			 (pe[i] << DP_TRAIN_PRE_EMPHASIS_SHIFT);
	ret = drm_dp_dpcd_write(&dp->aux, DP_TRAINING_LANE0_SET, buf, lanes);
	if (ret < 0)
		return ret;

	return 0;
}

static int dw_dp_link_configure(struct dw_dp *dp)
{
	struct dw_dp_link *link = &dp->link;
	union phy_configure_opts phy_cfg;
	u8 buf[2];
	int ret;

	/* Move PHY to P3 */
	regmap_update_bits(dp->regmap, DPTX_PHYIF_CTRL, PHY_POWERDOWN,
			   FIELD_PREP(PHY_POWERDOWN, 0x3));

	phy_cfg.dp.lanes = link->lanes;
	phy_cfg.dp.link_rate = link->rate / 100;
	phy_cfg.dp.ssc = link->caps.ssc;
	phy_cfg.dp.set_lanes = true;
	phy_cfg.dp.set_rate = true;
	phy_cfg.dp.set_voltages = false;
	ret = phy_configure(dp->phy, &phy_cfg);
	if (ret)
		return ret;

	regmap_update_bits(dp->regmap, DPTX_PHYIF_CTRL, PHY_LANES,
			   FIELD_PREP(PHY_LANES, link->lanes / 2));

	/* Move PHY to P0 */
	regmap_update_bits(dp->regmap, DPTX_PHYIF_CTRL, PHY_POWERDOWN,
			   FIELD_PREP(PHY_POWERDOWN, 0x0));

	dw_dp_phy_xmit_enable(dp, link->lanes);

	buf[0] = drm_dp_link_rate_to_bw_code(link->rate);
	buf[1] = link->lanes;

	if (link->caps.enhanced_framing) {
		buf[1] |= DP_LANE_COUNT_ENHANCED_FRAME_EN;
		regmap_update_bits(dp->regmap, DPTX_CCTL, ENHANCE_FRAMING_EN,
				   FIELD_PREP(ENHANCE_FRAMING_EN, 1));
	} else {
		regmap_update_bits(dp->regmap, DPTX_CCTL, ENHANCE_FRAMING_EN,
				   FIELD_PREP(ENHANCE_FRAMING_EN, 0));
	}

	ret = drm_dp_dpcd_write(&dp->aux, DP_LINK_BW_SET, buf, sizeof(buf));
	if (ret < 0)
		return ret;

	buf[0] = link->caps.ssc ? DP_SPREAD_AMP_0_5 : 0;
	buf[1] = link->caps.channel_coding ? DP_SET_ANSI_8B10B : 0;

	ret = drm_dp_dpcd_write(&dp->aux, DP_DOWNSPREAD_CTRL, buf,
				sizeof(buf));
	if (ret < 0)
		return ret;

	return 0;
}

static void dw_dp_link_train_init(struct drm_dp_link_train *train)
{
	struct drm_dp_link_train_set *request = &train->request;
	struct drm_dp_link_train_set *adjust = &train->adjust;
	unsigned int i;

	for (i = 0; i < 4; i++) {
		request->voltage_swing[i] = 0;
		adjust->voltage_swing[i] = 0;

		request->pre_emphasis[i] = 0;
		adjust->pre_emphasis[i] = 0;
	}

	train->clock_recovered = false;
	train->channel_equalized = false;
}

static bool dw_dp_link_train_valid(const struct drm_dp_link_train *train)
{
	return train->clock_recovered && train->channel_equalized;
}

static int dw_dp_link_train_set_pattern(struct dw_dp *dp, u32 pattern)
{
	u8 buf = 0;
	int ret;

	if (pattern && pattern != DP_TRAINING_PATTERN_4) {
		buf |= DP_LINK_SCRAMBLING_DISABLE;

		regmap_update_bits(dp->regmap, DPTX_CCTL, SCRAMBLE_DIS,
				   FIELD_PREP(SCRAMBLE_DIS, 1));
	} else {
		regmap_update_bits(dp->regmap, DPTX_CCTL, SCRAMBLE_DIS,
				   FIELD_PREP(SCRAMBLE_DIS, 0));
	}

	switch (pattern) {
	case DP_TRAINING_PATTERN_DISABLE:
		dw_dp_phy_set_pattern(dp, DPTX_PHY_PATTERN_NONE);
		break;
	case DP_TRAINING_PATTERN_1:
		dw_dp_phy_set_pattern(dp, DPTX_PHY_PATTERN_TPS_1);
		break;
	case DP_TRAINING_PATTERN_2:
		dw_dp_phy_set_pattern(dp, DPTX_PHY_PATTERN_TPS_2);
		break;
	case DP_TRAINING_PATTERN_3:
		dw_dp_phy_set_pattern(dp, DPTX_PHY_PATTERN_TPS_3);
		break;
	case DP_TRAINING_PATTERN_4:
		dw_dp_phy_set_pattern(dp, DPTX_PHY_PATTERN_TPS_4);
		break;
	default:
		return -EINVAL;
	}

	ret = drm_dp_dpcd_writeb(&dp->aux, DP_TRAINING_PATTERN_SET,
				 buf | pattern);
	if (ret < 0)
		return ret;

	return 0;
}

static void dw_dp_link_get_adjustments(struct dw_dp_link *link,
				       u8 status[DP_LINK_STATUS_SIZE])
{
	struct drm_dp_link_train_set *adjust = &link->train.adjust;
	unsigned int i;

	for (i = 0; i < link->lanes; i++) {
		adjust->voltage_swing[i] =
			drm_dp_get_adjust_request_voltage(status, i) >>
				DP_TRAIN_VOLTAGE_SWING_SHIFT;

		adjust->pre_emphasis[i] =
			drm_dp_get_adjust_request_pre_emphasis(status, i) >>
				DP_TRAIN_PRE_EMPHASIS_SHIFT;
	}
}

static void dw_dp_link_train_adjust(struct drm_dp_link_train *train)
{
	struct drm_dp_link_train_set *request = &train->request;
	struct drm_dp_link_train_set *adjust = &train->adjust;
	unsigned int i;

	for (i = 0; i < 4; i++)
		if (request->voltage_swing[i] != adjust->voltage_swing[i])
			request->voltage_swing[i] = adjust->voltage_swing[i];

	for (i = 0; i < 4; i++)
		if (request->pre_emphasis[i] != adjust->pre_emphasis[i])
			request->pre_emphasis[i] = adjust->pre_emphasis[i];
}

static int dw_dp_link_clock_recovery(struct dw_dp *dp)
{
	struct dw_dp_link *link = &dp->link;
	u8 status[DP_LINK_STATUS_SIZE];
	unsigned int tries = 0;
	int ret;

	ret = dw_dp_link_train_set_pattern(dp, DP_TRAINING_PATTERN_1);
	if (ret)
		return ret;

	for (;;) {
		ret = dw_dp_link_train_update_vs_emph(dp);
		if (ret)
			return ret;

		drm_dp_link_train_clock_recovery_delay(link->dpcd);

		ret = drm_dp_dpcd_read_link_status(&dp->aux, status);
		if (ret < 0) {
			dev_err(dp->dev, "failed to read link status: %d\n", ret);
			return ret;
		}

		if (drm_dp_clock_recovery_ok(status, link->lanes)) {
			link->train.clock_recovered = true;
			break;
		}

		dw_dp_link_get_adjustments(link, status);

		if (link->train.request.voltage_swing[0] ==
		    link->train.adjust.voltage_swing[0])
			tries++;
		else
			tries = 0;

		if (tries == 5)
			break;

		dw_dp_link_train_adjust(&link->train);
	}

	return 0;
}

static int dw_dp_link_channel_equalization(struct dw_dp *dp)
{
	struct dw_dp_link *link = &dp->link;
	u8 status[DP_LINK_STATUS_SIZE], pattern;
	unsigned int tries;
	int ret;

	if (link->caps.tps4_supported)
		pattern = DP_TRAINING_PATTERN_4;
	else if (link->caps.tps3_supported)
		pattern = DP_TRAINING_PATTERN_3;
	else
		pattern = DP_TRAINING_PATTERN_2;
	ret = dw_dp_link_train_set_pattern(dp, pattern);
	if (ret)
		return ret;

	for (tries = 1; tries < 5; tries++) {
		ret = dw_dp_link_train_update_vs_emph(dp);
		if (ret)
			return ret;

		drm_dp_link_train_channel_eq_delay(link->dpcd);

		ret = drm_dp_dpcd_read_link_status(&dp->aux, status);
		if (ret < 0)
			return ret;

		if (!drm_dp_clock_recovery_ok(status, link->lanes)) {
			dev_err(dp->dev, "clock recovery lost while equalizing channel\n");
			link->train.clock_recovered = false;
			break;
		}

		if (drm_dp_channel_eq_ok(status, link->lanes)) {
			link->train.channel_equalized = true;
			break;
		}

		dw_dp_link_get_adjustments(link, status);
		dw_dp_link_train_adjust(&link->train);
	}

	return 0;
}

static int dw_dp_link_downgrade(struct dw_dp *dp)
{
	struct dw_dp_link *link = &dp->link;
	struct dw_dp_video *video = &dp->video;

	switch (link->rate) {
	case 162000:
		return -EINVAL;
	case 270000:
		link->rate = 162000;
		break;
	case 540000:
		link->rate = 270000;
		break;
	case 810000:
		link->rate = 540000;
		break;
	}

	if (!dw_dp_bandwidth_ok(dp, &video->mode, video->bpp, link->lanes,
				link->rate))
		return -E2BIG;

	return 0;
}

static int dw_dp_link_train_full(struct dw_dp *dp)
{
	struct dw_dp_link *link = &dp->link;
	int ret;

retry:
	dw_dp_link_train_init(&link->train);

	dev_info(dp->dev, "full-training link: %u lane%s at %u MHz\n",
		 link->lanes, (link->lanes > 1) ? "s" : "", link->rate / 100);

	ret = dw_dp_link_configure(dp);
	if (ret < 0) {
		dev_err(dp->dev, "failed to configure DP link: %d\n", ret);
		return ret;
	}

	ret = dw_dp_link_clock_recovery(dp);
	if (ret < 0) {
		dev_err(dp->dev, "clock recovery failed: %d\n", ret);
		goto out;
	}

	if (!link->train.clock_recovered) {
		dev_err(dp->dev, "clock recovery failed, downgrading link\n");

		ret = dw_dp_link_downgrade(dp);
		if (ret < 0)
			goto out;
		else
			goto retry;
	}

	dev_info(dp->dev, "clock recovery succeeded\n");

	ret = dw_dp_link_channel_equalization(dp);
	if (ret < 0) {
		dev_err(dp->dev, "channel equalization failed: %d\n", ret);
		goto out;
	}

	if (!link->train.channel_equalized) {
		dev_err(dp->dev, "channel equalization failed, downgrading link\n");

		ret = dw_dp_link_downgrade(dp);
		if (ret < 0)
			goto out;
		else
			goto retry;
	}

	dev_info(dp->dev, "channel equalization succeeded\n");

out:
	dw_dp_link_train_set_pattern(dp, DP_TRAINING_PATTERN_DISABLE);
	return ret;
}

static int dw_dp_link_train_fast(struct dw_dp *dp)
{
	struct dw_dp_link *link = &dp->link;
	u8 status[DP_LINK_STATUS_SIZE], pattern;
	int ret;

	dw_dp_link_train_init(&link->train);

	dev_info(dp->dev, "fast-training link: %u lane%s at %u MHz\n",
		 link->lanes, (link->lanes > 1) ? "s" : "", link->rate / 100);

	ret = dw_dp_link_configure(dp);
	if (ret < 0) {
		dev_err(dp->dev, "failed to configure DP link: %d\n", ret);
		return ret;
	}

	ret = dw_dp_link_train_set_pattern(dp, DP_TRAINING_PATTERN_1);
	if (ret)
		goto out;

	usleep_range(500, 1000);

	if (link->caps.tps4_supported)
		pattern = DP_TRAINING_PATTERN_4;
	else if (link->caps.tps3_supported)
		pattern = DP_TRAINING_PATTERN_3;
	else
		pattern = DP_TRAINING_PATTERN_2;
	ret = dw_dp_link_train_set_pattern(dp, pattern);
	if (ret)
		goto out;

	usleep_range(500, 1000);

	ret = drm_dp_dpcd_read_link_status(&dp->aux, status);
	if (ret < 0) {
		dev_err(dp->dev, "failed to read link status: %d\n", ret);
		goto out;
	}

	if (!drm_dp_clock_recovery_ok(status, link->lanes)) {
		dev_err(dp->dev, "clock recovery failed\n");
		ret = -EIO;
		goto out;
	}

	if (!drm_dp_channel_eq_ok(status, link->lanes)) {
		dev_err(dp->dev, "channel equalization failed\n");
		ret = -EIO;
		goto out;
	}

out:
	dw_dp_link_train_set_pattern(dp, DP_TRAINING_PATTERN_DISABLE);
	return ret;
}

static int dw_dp_link_train(struct dw_dp *dp)
{
	struct dw_dp_link *link = &dp->link;
	int ret;

	if (link->caps.fast_training) {
		if (dw_dp_link_train_valid(&link->train)) {
			ret = dw_dp_link_train_fast(dp);
			if (ret < 0)
				dev_err(dp->dev,
					"fast link training failed: %d\n", ret);
			else
				return 0;
		}
	}

	ret = dw_dp_link_train_full(dp);
	if (ret < 0) {
		dev_err(dp->dev, "full link training failed: %d\n", ret);
		return ret;
	}

	return 0;
}

static int dw_dp_send_sdp(struct dw_dp *dp, struct dw_dp_sdp *sdp)
{
	const u8 *payload = sdp->db;
	u32 reg;
	int i, nr;

	nr = find_first_zero_bit(dp->sdp_reg_bank, SDP_REG_BANK_SIZE);
	if (nr < SDP_REG_BANK_SIZE)
		set_bit(nr, dp->sdp_reg_bank);
	else
		return -EBUSY;

	reg = DPTX_SDP_REGISTER_BANK + nr * 9 * 4;

	/* SDP header */
	regmap_write(dp->regmap, reg, get_unaligned_le32(&sdp->header));

	/* SDP data payload */
	for (i = 1; i < 9; i++, payload += 4)
		regmap_write(dp->regmap, reg + i * 4,
			     FIELD_PREP(SDP_REGS, get_unaligned_le32(payload)));

	if (sdp->flags & DPTX_SDP_VERTICAL_INTERVAL)
		regmap_update_bits(dp->regmap, DPTX_SDP_VERTICAL_CTRL,
				   EN_VERTICAL_SDP << nr,
				   EN_VERTICAL_SDP << nr);

	if (sdp->flags & DPTX_SDP_HORIZONTAL_INTERVAL)
		regmap_update_bits(dp->regmap, DPTX_SDP_HORIZONTAL_CTRL,
				   EN_HORIZONTAL_SDP << nr,
				   EN_HORIZONTAL_SDP << nr);

	return 0;
}

static void dw_dp_vsc_sdp_pack(const struct drm_dp_vsc_sdp *vsc,
			       struct dw_dp_sdp *sdp)
{
	sdp->header.HB0 = 0;
	sdp->header.HB1 = DP_SDP_VSC;
	sdp->header.HB2 = vsc->revision;
	sdp->header.HB3 = vsc->length;

	sdp->db[16] = (vsc->pixelformat & 0xf) << 4;
	sdp->db[16] |= vsc->colorimetry & 0xf;

	switch (vsc->bpc) {
	case 8:
		sdp->db[17] = 0x1;
		break;
	case 10:
		sdp->db[17] = 0x2;
		break;
	case 12:
		sdp->db[17] = 0x3;
		break;
	case 16:
		sdp->db[17] = 0x4;
		break;
	case 6:
	default:
		break;
	}

	if (vsc->dynamic_range == DP_DYNAMIC_RANGE_CTA)
		sdp->db[17] |= 0x80;

	sdp->db[18] = vsc->content_type & 0x7;

	sdp->flags |= DPTX_SDP_VERTICAL_INTERVAL;
}

static int dw_dp_send_vsc_sdp(struct dw_dp *dp)
{
	struct dw_dp_video *video = &dp->video;
	struct drm_dp_vsc_sdp vsc = {};
	struct dw_dp_sdp sdp = {};

	vsc.revision = 0x5;
	vsc.length = 0x13;

	switch (video->color_format) {
	case DRM_COLOR_FORMAT_YCRCB444:
		vsc.pixelformat = DP_PIXELFORMAT_YUV444;
		break;
	case DRM_COLOR_FORMAT_YCRCB420:
		vsc.pixelformat = DP_PIXELFORMAT_YUV420;
		break;
	case DRM_COLOR_FORMAT_YCRCB422:
		vsc.pixelformat = DP_PIXELFORMAT_YUV422;
		break;
	case DRM_COLOR_FORMAT_RGB444:
	default:
		vsc.pixelformat = DP_PIXELFORMAT_RGB;
		break;
	}

	if (video->color_format == DRM_COLOR_FORMAT_RGB444)
		vsc.colorimetry = DP_COLORIMETRY_DEFAULT;
	else
		vsc.colorimetry = DP_COLORIMETRY_BT709_YCC;

	vsc.bpc = video->bpc;
	vsc.dynamic_range = DP_DYNAMIC_RANGE_CTA;
	vsc.content_type = DP_CONTENT_TYPE_NOT_DEFINED;

	dw_dp_vsc_sdp_pack(&vsc, &sdp);

	return dw_dp_send_sdp(dp, &sdp);
}

static int dw_dp_video_set_pixel_mode(struct dw_dp *dp, u8 pixel_mode)
{
	switch (pixel_mode) {
	case DPTX_MP_SINGLE_PIXEL:
	case DPTX_MP_DUAL_PIXEL:
	case DPTX_MP_QUAD_PIXEL:
		break;
	default:
		return -EINVAL;
	}

	regmap_update_bits(dp->regmap, DPTX_VSAMPLE_CTRL, PIXEL_MODE_SELECT,
			   FIELD_PREP(PIXEL_MODE_SELECT, pixel_mode));

	return 0;
}

static int dw_dp_video_set_msa(struct dw_dp *dp, u8 color_format, u8 bpc,
			       u16 vstart, u16 hstart)
{
	struct dw_dp_link *link = &dp->link;
	u16 misc = 0;

	if (link->vsc_sdp_extension_for_colorimetry_supported)
		misc |= DP_MSA_MISC_COLOR_VSC_SDP;

	switch (color_format) {
	case DRM_COLOR_FORMAT_RGB444:
		misc |= DP_MSA_MISC_COLOR_RGB;
		break;
	case DRM_COLOR_FORMAT_YCRCB444:
		misc |= DP_MSA_MISC_COLOR_YCBCR_444_BT709;
		break;
	case DRM_COLOR_FORMAT_YCRCB422:
		misc |= DP_MSA_MISC_COLOR_YCBCR_422_BT709;
		break;
	case DRM_COLOR_FORMAT_YCRCB420:
		break;
	default:
		return -EINVAL;
	}

	switch (bpc) {
	case 6:
		misc |= DP_MSA_MISC_6_BPC;
		break;
	case 8:
		misc |= DP_MSA_MISC_8_BPC;
		break;
	case 10:
		misc |= DP_MSA_MISC_10_BPC;
		break;
	case 12:
		misc |= DP_MSA_MISC_12_BPC;
		break;
	case 16:
		misc |= DP_MSA_MISC_16_BPC;
		break;
	default:
		return -EINVAL;
	}

	regmap_write(dp->regmap, DPTX_VIDEO_MSA1,
		     FIELD_PREP(VSTART, vstart) | FIELD_PREP(HSTART, hstart));
	regmap_write(dp->regmap, DPTX_VIDEO_MSA2, FIELD_PREP(MISC0, misc));
	regmap_write(dp->regmap, DPTX_VIDEO_MSA3, FIELD_PREP(MISC1, misc >> 8));

	return 0;
}

static void dw_dp_video_disable(struct dw_dp *dp)
{
	regmap_update_bits(dp->regmap, DPTX_VSAMPLE_CTRL, VIDEO_STREAM_ENABLE,
			   FIELD_PREP(VIDEO_STREAM_ENABLE, 0));
}

static int dw_dp_video_enable(struct dw_dp *dp)
{
	struct dw_dp_video *video = &dp->video;
	struct dw_dp_link *link = &dp->link;
	struct drm_display_mode *mode = &video->mode;
	u8 color_format = video->color_format;
	u8 bpc = video->bpc;
	u8 pixel_mode = video->pixel_mode;
	u8 bpp = video->bpp, init_threshold, vic;
	u32 hactive, hblank, h_sync_width, h_front_porch;
	u32 vactive, vblank, v_sync_width, v_front_porch;
	u32 vstart = mode->vtotal - mode->vsync_start;
	u32 hstart = mode->htotal - mode->hsync_start;
	u32 peak_stream_bandwidth, link_bandwidth;
	u32 average_bytes_per_tu, average_bytes_per_tu_frac;
	u32 ts, hblank_interval;
	u32 value;
	int ret;

	ret = dw_dp_video_set_pixel_mode(dp, pixel_mode);
	if (ret)
		return ret;

	ret = dw_dp_video_set_msa(dp, color_format, bpc, vstart, hstart);
	if (ret)
		return ret;

	regmap_update_bits(dp->regmap, DPTX_VSAMPLE_CTRL, VIDEO_MAPPING,
			   FIELD_PREP(VIDEO_MAPPING, video->video_mapping));

	/* Configure DPTX_VINPUT_POLARITY_CTRL register */
	value = 0;
	if (mode->flags & DRM_MODE_FLAG_PHSYNC)
		value |= FIELD_PREP(HSYNC_IN_POLARITY, 1);
	if (mode->flags & DRM_MODE_FLAG_PVSYNC)
		value |= FIELD_PREP(VSYNC_IN_POLARITY, 1);
	regmap_write(dp->regmap, DPTX_VINPUT_POLARITY_CTRL, value);

	/* Configure DPTX_VIDEO_CONFIG1 register */
	hactive = mode->hdisplay;
	hblank = mode->htotal - mode->hdisplay;
	value = FIELD_PREP(HACTIVE, hactive) | FIELD_PREP(HBLANK, hblank);
	if (mode->flags & DRM_MODE_FLAG_INTERLACE)
		value |= FIELD_PREP(I_P, 1);
	vic = drm_match_cea_mode(mode);
	if (vic == 5 || vic == 6 || vic == 7 ||
	    vic == 10 || vic == 11 || vic == 20 ||
	    vic == 21 || vic == 22 || vic == 39 ||
	    vic == 25 || vic == 26 || vic == 40 ||
	    vic == 44 || vic == 45 || vic == 46 ||
	    vic == 50 || vic == 51 || vic == 54 ||
	    vic == 55 || vic == 58 || vic  == 59)
		value |= R_V_BLANK_IN_OSC;
	regmap_write(dp->regmap, DPTX_VIDEO_CONFIG1, value);

	/* Configure DPTX_VIDEO_CONFIG2 register */
	vblank = mode->vtotal - mode->vdisplay;
	vactive = mode->vdisplay;
	regmap_write(dp->regmap, DPTX_VIDEO_CONFIG2,
		     FIELD_PREP(VBLANK, vblank) | FIELD_PREP(VACTIVE, vactive));

	/* Configure DPTX_VIDEO_CONFIG3 register */
	h_sync_width = mode->hsync_end - mode->hsync_start;
	h_front_porch = mode->hsync_start - mode->hdisplay;
	regmap_write(dp->regmap, DPTX_VIDEO_CONFIG3,
		     FIELD_PREP(H_SYNC_WIDTH, h_sync_width) |
		     FIELD_PREP(H_FRONT_PORCH, h_front_porch));

	/* Configure DPTX_VIDEO_CONFIG4 register */
	v_sync_width = mode->vsync_end - mode->vsync_start;
	v_front_porch = mode->vsync_start - mode->vdisplay;
	regmap_write(dp->regmap, DPTX_VIDEO_CONFIG4,
		     FIELD_PREP(V_SYNC_WIDTH, v_sync_width) |
		     FIELD_PREP(V_FRONT_PORCH, v_front_porch));

	/* Configure DPTX_VIDEO_CONFIG5 register */
	peak_stream_bandwidth = mode->clock * bpp / 8;
	link_bandwidth = (link->rate / 1000) * link->lanes;
	ts = peak_stream_bandwidth * 64 / link_bandwidth;
	average_bytes_per_tu = ts / 1000;
	average_bytes_per_tu_frac = ts / 100 - average_bytes_per_tu * 10;
	if (pixel_mode == DPTX_MP_SINGLE_PIXEL) {
		if (average_bytes_per_tu < 6)
			init_threshold = 32;
		else if (hblank <= 80 && color_format != DRM_COLOR_FORMAT_YCRCB420)
			init_threshold = 12;
		else if (hblank <= 40 && color_format == DRM_COLOR_FORMAT_YCRCB420)
			init_threshold = 3;
		else
			init_threshold = 16;
	} else {
		u32 t1 = 0, t2 = 0, t3 = 0;

		switch (bpc) {
		case 6:
			t1 = (4 * 1000 / 9) * link->lanes;
			break;
		case 8:
			if (color_format == DRM_COLOR_FORMAT_YCRCB422) {
				t1 = (1000 / 2) * link->lanes;
			} else {
				if (pixel_mode == DPTX_MP_DUAL_PIXEL)
					t1 = (1000 / 3) * link->lanes;
				else
					t1 = (3000 / 16) * link->lanes;
			}
			break;
		case 10:
			if (color_format == DRM_COLOR_FORMAT_YCRCB422)
				t1 = (2000 / 5) * link->lanes;
			else
				t1 = (4000 / 15) * link->lanes;
			break;
		case 12:
			if (color_format == DRM_COLOR_FORMAT_YCRCB422) {
				if (pixel_mode == DPTX_MP_DUAL_PIXEL)
					t1 = (1000 / 6) * link->lanes;
				else
					t1 = (1000 / 3) * link->lanes;
			} else {
				t1 = (2000 / 9) * link->lanes;
			}
			break;
		case 16:
			if (color_format != DRM_COLOR_FORMAT_YCRCB422 &&
			    pixel_mode == DPTX_MP_DUAL_PIXEL)
				t1 = (1000 / 6) * link->lanes;
			else
				t1 = (1000 / 4) * link->lanes;
			break;
		default:
			return -EINVAL;
		}

		if (color_format == DRM_COLOR_FORMAT_YCRCB420)
			t2 = (link->rate / 4) * 1000 / (mode->clock / 2);
		else
			t2 = (link->rate / 4) * 1000 / mode->clock;

		if (average_bytes_per_tu_frac)
			t3 = average_bytes_per_tu + 1;
		else
			t3 = average_bytes_per_tu;
		init_threshold = t1 * t2 * t3 / (1000 * 1000);
		if (init_threshold <= 16 || average_bytes_per_tu < 10)
			init_threshold = 40;
	}

	regmap_write(dp->regmap, DPTX_VIDEO_CONFIG5,
		     FIELD_PREP(INIT_THRESHOLD_HI, init_threshold >> 6) |
		     FIELD_PREP(AVERAGE_BYTES_PER_TU_FRAC, average_bytes_per_tu_frac) |
		     FIELD_PREP(INIT_THRESHOLD, init_threshold) |
		     FIELD_PREP(AVERAGE_BYTES_PER_TU, average_bytes_per_tu));

	/* Configure DPTX_VIDEO_HBLANK_INTERVAL register */
	hblank_interval = hblank * (link->rate / 4) / mode->clock;
	regmap_write(dp->regmap, DPTX_VIDEO_HBLANK_INTERVAL,
		     FIELD_PREP(HBLANK_INTERVAL_EN, 1) |
		     FIELD_PREP(HBLANK_INTERVAL, hblank_interval));

	/* Video stream enable */
	regmap_update_bits(dp->regmap, DPTX_VSAMPLE_CTRL, VIDEO_STREAM_ENABLE,
			   FIELD_PREP(VIDEO_STREAM_ENABLE, 1));

	if (link->vsc_sdp_extension_for_colorimetry_supported)
		dw_dp_send_vsc_sdp(dp);

	return 0;
}

static irqreturn_t dw_dp_hpd_irq_handler(int irq, void *arg)
{
	struct dw_dp *dp = arg;
	bool hpd = dw_dp_detect(dp);

	mutex_lock(&dp->irq_lock);

	dp->hotplug.long_hpd = true;

	if (dp->hotplug.status && !hpd) {
		usleep_range(2000, 2001);

		hpd = dw_dp_detect(dp);
		if (hpd)
			dp->hotplug.long_hpd = false;
	}

	dp->hotplug.status = hpd;

	mutex_unlock(&dp->irq_lock);

	schedule_work(&dp->hpd_work);

	return IRQ_HANDLED;
}

static void dw_dp_hpd_init(struct dw_dp *dp)
{
	dp->hotplug.status = dw_dp_detect(dp);

	if (dp->hpd_gpio) {
		regmap_update_bits(dp->regmap, DPTX_CCTL, FORCE_HPD,
				   FIELD_PREP(FORCE_HPD, 1));
		return;
	}

	/* Enable all HPD interrupts */
	regmap_update_bits(dp->regmap, DPTX_HPD_INTERRUPT_ENABLE,
			   HPD_UNPLUG_EN | HPD_PLUG_EN | HPD_IRQ_EN,
			   FIELD_PREP(HPD_UNPLUG_EN, 1) |
			   FIELD_PREP(HPD_PLUG_EN, 1) |
			   FIELD_PREP(HPD_IRQ_EN, 1));

	/* Enable all top-level interrupts */
	regmap_update_bits(dp->regmap, DPTX_GENERAL_INTERRUPT_ENABLE,
			   HPD_EVENT_EN, FIELD_PREP(HPD_EVENT_EN, 1));
}

static void dw_dp_aux_init(struct dw_dp *dp)
{
	regmap_update_bits(dp->regmap, DPTX_SOFT_RESET_CTRL, AUX_RESET,
			   FIELD_PREP(AUX_RESET, 1));
	usleep_range(10, 20);
	regmap_update_bits(dp->regmap, DPTX_SOFT_RESET_CTRL, AUX_RESET,
			   FIELD_PREP(AUX_RESET, 0));

	regmap_update_bits(dp->regmap, DPTX_GENERAL_INTERRUPT_ENABLE,
			   AUX_REPLY_EVENT_EN,
			   FIELD_PREP(AUX_REPLY_EVENT_EN, 1));
}

static void dw_dp_init(struct dw_dp *dp)
{
	regmap_update_bits(dp->regmap, DPTX_SOFT_RESET_CTRL, CONTROLLER_RESET,
			   FIELD_PREP(CONTROLLER_RESET, 1));
	usleep_range(10, 20);
	regmap_update_bits(dp->regmap, DPTX_SOFT_RESET_CTRL, CONTROLLER_RESET,
			   FIELD_PREP(CONTROLLER_RESET, 0));

	regmap_update_bits(dp->regmap, DPTX_SOFT_RESET_CTRL, PHY_SOFT_RESET,
			   FIELD_PREP(PHY_SOFT_RESET, 1));
	usleep_range(10, 20);
	regmap_update_bits(dp->regmap, DPTX_SOFT_RESET_CTRL, PHY_SOFT_RESET,
			   FIELD_PREP(PHY_SOFT_RESET, 0));

	regmap_update_bits(dp->regmap, DPTX_CCTL, DEFAULT_FAST_LINK_TRAIN_EN,
			   FIELD_PREP(DEFAULT_FAST_LINK_TRAIN_EN, 0));

	dw_dp_hpd_init(dp);
	dw_dp_aux_init(dp);
}

static void dw_dp_encoder_enable(struct drm_encoder *encoder)
{

}

static void dw_dp_encoder_disable(struct drm_encoder *encoder)
{
	struct dw_dp *dp = encoder_to_dp(encoder);
	struct drm_crtc *crtc = encoder->crtc;
	struct rockchip_crtc_state *s = to_rockchip_crtc_state(crtc->state);

	if (!crtc->state->active_changed)
		return;

	if (dp->split_mode)
		s->output_if &= ~(VOP_OUTPUT_IF_DP0 | VOP_OUTPUT_IF_DP1);
	else
		s->output_if &= ~(dp->id ? VOP_OUTPUT_IF_DP1 : VOP_OUTPUT_IF_DP0);
}

static int dw_dp_encoder_atomic_check(struct drm_encoder *encoder,
				      struct drm_crtc_state *crtc_state,
				      struct drm_connector_state *conn_state)
{
	struct dw_dp *dp = encoder_to_dp(encoder);
	struct dw_dp_video *video = &dp->video;
	struct rockchip_crtc_state *s = to_rockchip_crtc_state(crtc_state);
	struct drm_display_info *di = &conn_state->connector->display_info;

	switch (video->color_format) {
	case DRM_COLOR_FORMAT_YCRCB420:
		s->output_mode = ROCKCHIP_OUT_MODE_YUV420;
		break;
	case DRM_COLOR_FORMAT_YCRCB422:
		s->output_mode = ROCKCHIP_OUT_MODE_S888_DUMMY;
		break;
	case DRM_COLOR_FORMAT_RGB444:
	case DRM_COLOR_FORMAT_YCRCB444:
	default:
		s->output_mode = ROCKCHIP_OUT_MODE_AAAA;
		break;
	}

	if (dp->split_mode) {
		s->output_flags |= ROCKCHIP_OUTPUT_DUAL_CHANNEL_LEFT_RIGHT_MODE;
		s->output_flags |= dp->id ? ROCKCHIP_OUTPUT_DATA_SWAP : 0;
		s->output_if |= VOP_OUTPUT_IF_DP0 | VOP_OUTPUT_IF_DP1;
	} else {
		s->output_if |= dp->id ? VOP_OUTPUT_IF_DP1 : VOP_OUTPUT_IF_DP0;
	}

	s->output_type = DRM_MODE_CONNECTOR_DisplayPort;
	s->bus_format = video->bus_format;
	s->bus_flags = di->bus_flags;
	s->tv_state = &conn_state->tv;
	s->eotf = HDMI_EOTF_TRADITIONAL_GAMMA_SDR;
	s->color_space = V4L2_COLORSPACE_DEFAULT;

	return 0;
}

static const struct drm_encoder_helper_funcs dw_dp_encoder_helper_funcs = {
	.enable			= dw_dp_encoder_enable,
	.disable		= dw_dp_encoder_disable,
	.atomic_check		= dw_dp_encoder_atomic_check,
};

static int dw_dp_aux_write_data(struct dw_dp *dp, const u8 *buffer, size_t size)
{
	size_t i, j;

	for (i = 0; i < DIV_ROUND_UP(size, 4); i++) {
		size_t num = min_t(size_t, size - i * 4, 4);
		u32 value = 0;

		for (j = 0; j < num; j++)
			value |= buffer[i * 4 + j] << (j * 8);

		regmap_write(dp->regmap, DPTX_AUX_DATA0 + i * 4, value);
	}

	return size;
}

static int dw_dp_aux_read_data(struct dw_dp *dp, u8 *buffer, size_t size)
{
	size_t i, j;

	for (i = 0; i < DIV_ROUND_UP(size, 4); i++) {
		size_t num = min_t(size_t, size - i * 4, 4);
		u32 value;

		regmap_read(dp->regmap, DPTX_AUX_DATA0 + i * 4, &value);

		for (j = 0; j < num; j++)
			buffer[i * 4 + j] = value >> (j * 8);
	}

	return size;
}

static ssize_t dw_dp_aux_transfer(struct drm_dp_aux *aux,
				  struct drm_dp_aux_msg *msg)
{
	struct dw_dp *dp = container_of(aux, struct dw_dp, aux);
	unsigned long timeout = msecs_to_jiffies(250);
	u32 status, value;
	ssize_t ret = 0;

	if (WARN_ON(msg->size > 16))
		return -E2BIG;

	switch (msg->request & ~DP_AUX_I2C_MOT) {
	case DP_AUX_NATIVE_WRITE:
	case DP_AUX_I2C_WRITE:
	case DP_AUX_I2C_WRITE_STATUS_UPDATE:
		ret = dw_dp_aux_write_data(dp, msg->buffer, msg->size);
		if (ret < 0)
			return ret;
		break;
	case DP_AUX_NATIVE_READ:
	case DP_AUX_I2C_READ:
		break;
	default:
		return -EINVAL;
	}

	if (msg->size > 0)
		value = FIELD_PREP(AUX_LEN_REQ, msg->size - 1);
	else
		value = FIELD_PREP(I2C_ADDR_ONLY, 1);
	value |= FIELD_PREP(AUX_CMD_TYPE, msg->request);
	value |= FIELD_PREP(AUX_ADDR, msg->address);
	regmap_write(dp->regmap, DPTX_AUX_CMD, value);

	status = wait_for_completion_timeout(&dp->complete, timeout);
	if (!status) {
		dev_err(dp->dev, "timeout waiting for AUX reply\n");
		return -ETIMEDOUT;
	}

	regmap_read(dp->regmap, DPTX_AUX_STATUS, &value);
	if (value & AUX_TIMEOUT)
		return -ETIMEDOUT;

	msg->reply = FIELD_GET(AUX_STATUS, value);

	if (msg->size > 0 && msg->reply == DP_AUX_NATIVE_REPLY_ACK) {
		if (msg->request & DP_AUX_I2C_READ) {
			size_t count = FIELD_GET(AUX_BYTES_READ, value) - 1;

			if (count != msg->size)
				return -EBUSY;

			ret = dw_dp_aux_read_data(dp, msg->buffer, count);
			if (ret < 0)
				return ret;
		}
	}

	return ret;
}

static int dw_dp_bridge_mode_valid(struct drm_bridge *bridge,
				   const struct drm_display_info *info,
				   const struct drm_display_mode *mode)
{
	struct dw_dp *dp = bridge_to_dp(bridge);
	struct dw_dp_link *link = &dp->link;
	struct drm_display_mode m;
	u32 min_bpp;

	if (info->color_formats & DRM_COLOR_FORMAT_YCRCB420 &&
	    link->vsc_sdp_extension_for_colorimetry_supported)
		min_bpp = 12;
	else if (info->color_formats & DRM_COLOR_FORMAT_YCRCB422)
		min_bpp = 16;
	else if (info->color_formats & DRM_COLOR_FORMAT_RGB444)
		min_bpp = 18;
	else
		min_bpp = 24;

	drm_mode_copy(&m, mode);

	if (dp->split_mode)
		drm_mode_convert_to_origin_mode(&m);

	if (!dw_dp_bandwidth_ok(dp, &m, min_bpp, link->lanes, link->rate))
		return MODE_CLOCK_HIGH;

	return MODE_OK;
}

static int dw_dp_bridge_attach(struct drm_bridge *bridge,
			       enum drm_bridge_attach_flags flags)
{
	struct dw_dp *dp = bridge_to_dp(bridge);
	struct drm_connector *connector = &dp->connector;
	int ret;

	if (flags & DRM_BRIDGE_ATTACH_NO_CONNECTOR)
		return 0;

	if (!bridge->encoder) {
		DRM_DEV_ERROR(dp->dev, "Parent encoder object not found");
		return -ENODEV;
	}

	connector->polled = DRM_CONNECTOR_POLL_HPD;
	connector->ycbcr_420_allowed = true;

	ret = drm_connector_init(bridge->dev, connector,
				 &dw_dp_connector_funcs,
				 DRM_MODE_CONNECTOR_DisplayPort);
	if (ret) {
		DRM_DEV_ERROR(dp->dev, "Failed to initialize connector\n");
		return ret;
	}

	drm_connector_helper_add(connector,
				 &dw_dp_connector_helper_funcs);

	drm_connector_attach_encoder(connector, bridge->encoder);

	return 0;
}

static void dw_dp_bridge_detach(struct drm_bridge *bridge)
{
	struct dw_dp *dp = bridge_to_dp(bridge);

	drm_connector_cleanup(&dp->connector);
}

static void dw_dp_bridge_atomic_pre_enable(struct drm_bridge *bridge,
					   struct drm_bridge_state *bridge_state)
{
	struct dw_dp *dp = bridge_to_dp(bridge);
	struct dw_dp_video *video = &dp->video;
	struct drm_crtc_state *crtc_state = bridge->encoder->crtc->state;
	struct drm_display_mode *m = &video->mode;

	drm_mode_copy(m, &crtc_state->adjusted_mode);

	if (dp->split_mode)
		drm_mode_convert_to_origin_mode(m);
}

static bool dw_dp_needs_link_retrain(struct dw_dp *dp)
{
	struct dw_dp_link *link = &dp->link;
	u8 link_status[DP_LINK_STATUS_SIZE];

	if (!dw_dp_link_train_valid(&link->train))
		return false;

	if (drm_dp_dpcd_read_link_status(&dp->aux, link_status) < 0)
		return false;

	/* Retrain if Channel EQ or CR not ok */
	return !drm_dp_channel_eq_ok(link_status, dp->link.lanes);
}

static void dw_dp_link_disable(struct dw_dp *dp)
{
	struct dw_dp_link *link = &dp->link;

	if (dw_dp_detect(dp))
		dw_dp_link_power_down(dp);

	dw_dp_phy_xmit_enable(dp, 0);

	phy_power_off(dp->phy);

	link->train.clock_recovered = false;
	link->train.channel_equalized = false;
}

static int dw_dp_link_enable(struct dw_dp *dp)
{
	int ret;

	ret = phy_power_on(dp->phy);
	if (ret)
		return ret;

	ret = dw_dp_link_power_up(dp);
	if (ret < 0)
		return ret;

	ret = dw_dp_link_train(dp);
	if (ret < 0) {
		dev_err(dp->dev, "link training failed: %d\n", ret);
		return ret;
	}

	return 0;
}

static void dw_dp_bridge_atomic_enable(struct drm_bridge *bridge,
				       struct drm_bridge_state *old_state)
{
	struct dw_dp *dp = bridge_to_dp(bridge);
	int ret;

	set_bit(0, dp->sdp_reg_bank);

	ret = dw_dp_link_enable(dp);
	if (ret < 0) {
		dev_err(dp->dev, "failed to enable link: %d\n", ret);
		return;
	}

	ret = dw_dp_video_enable(dp);
	if (ret < 0) {
		dev_err(dp->dev, "failed to enable video: %d\n", ret);
		return;
	}
}

static void dw_dp_bridge_atomic_disable(struct drm_bridge *bridge,
					struct drm_bridge_state *old_bridge_state)
{
	struct dw_dp *dp = bridge_to_dp(bridge);

	dw_dp_video_disable(dp);
	dw_dp_link_disable(dp);
	bitmap_zero(dp->sdp_reg_bank, SDP_REG_BANK_SIZE);
}

static enum drm_connector_status dw_dp_detect_dpcd(struct dw_dp *dp)
{
	int ret;

	ret = phy_power_on(dp->phy);
	if (ret)
		return ret;

	ret = dw_dp_link_probe(dp);
	if (ret) {
		phy_power_off(dp->phy);
		dev_err(dp->dev, "failed to probe DP link: %d\n", ret);
		return connector_status_disconnected;
	}

	phy_power_off(dp->phy);

	return connector_status_connected;
}

static enum drm_connector_status dw_dp_bridge_detect(struct drm_bridge *bridge)
{
	struct dw_dp *dp = bridge_to_dp(bridge);
	enum drm_connector_status status;

	if (dw_dp_detect(dp))
		status = dw_dp_detect_dpcd(dp);
	else
		status = connector_status_disconnected;

	if (status == connector_status_connected)
		extcon_set_state_sync(dp->extcon, EXTCON_DISP_DP, true);
	else
		extcon_set_state_sync(dp->extcon, EXTCON_DISP_DP, false);

	return status;
}

static struct edid *dw_dp_bridge_get_edid(struct drm_bridge *bridge,
					  struct drm_connector *connector)
{
	struct dw_dp *dp = bridge_to_dp(bridge);
	struct edid *edid;
	int ret;

	ret = phy_power_on(dp->phy);
	if (ret)
		return NULL;

	edid = drm_get_edid(connector, &dp->aux.ddc);

	phy_power_off(dp->phy);

	return edid;
}

static u32 *dw_dp_bridge_atomic_get_output_bus_fmts(struct drm_bridge *bridge,
					struct drm_bridge_state *bridge_state,
					struct drm_crtc_state *crtc_state,
					struct drm_connector_state *conn_state,
					unsigned int *num_output_fmts)
{
	struct dw_dp *dp = bridge_to_dp(bridge);
	struct dw_dp_link *link = &dp->link;
	struct drm_display_info *di = &conn_state->connector->display_info;
	struct drm_display_mode mode = crtc_state->mode;
	u32 *output_fmts;
	unsigned int i, j = 0;

	if (dp->split_mode)
		drm_mode_convert_to_origin_mode(&mode);

	*num_output_fmts = 0;

	output_fmts = kcalloc(ARRAY_SIZE(possible_output_fmts),
			      sizeof(*output_fmts), GFP_KERNEL);
	if (!output_fmts)
		return NULL;

	for (i = 0; i < ARRAY_SIZE(possible_output_fmts); i++) {
		const struct dw_dp_output_format *fmt = &possible_output_fmts[i];

		if (fmt->bpc > conn_state->max_bpc)
			continue;

		if (!(di->color_formats & fmt->color_format))
			continue;

		if (fmt->color_format == DRM_COLOR_FORMAT_YCRCB420 &&
		    !link->vsc_sdp_extension_for_colorimetry_supported)
			continue;

		if (drm_mode_is_420_only(di, &mode) &&
		    fmt->color_format != DRM_COLOR_FORMAT_YCRCB420)
			continue;

		if (!dw_dp_bandwidth_ok(dp, &mode, fmt->bpp, link->lanes, link->rate))
			continue;

		output_fmts[j++] = fmt->bus_format;
	}

	*num_output_fmts = j;

	return output_fmts;
}

static int dw_dp_bridge_atomic_check(struct drm_bridge *bridge,
				     struct drm_bridge_state *bridge_state,
				     struct drm_crtc_state *crtc_state,
				     struct drm_connector_state *conn_state)
{
	struct dw_dp *dp = bridge_to_dp(bridge);
	struct dw_dp_video *video = &dp->video;
	const struct dw_dp_output_format *fmt =
		dw_dp_get_output_format(bridge_state->output_bus_cfg.format);

	dev_dbg(dp->dev, "input format 0x%04x, output format 0x%04x\n",
		bridge_state->input_bus_cfg.format,
		bridge_state->output_bus_cfg.format);

	video->video_mapping = fmt->video_mapping;
	video->color_format = fmt->color_format;
	video->bus_format = fmt->bus_format;
	video->bpc = fmt->bpc;
	video->bpp = fmt->bpp;

	return 0;
}

static const struct drm_bridge_funcs dw_dp_bridge_funcs = {
	.atomic_duplicate_state = drm_atomic_helper_bridge_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_bridge_destroy_state,
	.atomic_reset = drm_atomic_helper_bridge_reset,
	.atomic_get_input_bus_fmts = drm_atomic_helper_bridge_propagate_bus_fmt,
	.atomic_get_output_bus_fmts = dw_dp_bridge_atomic_get_output_bus_fmts,
	.attach = dw_dp_bridge_attach,
	.detach = dw_dp_bridge_detach,
	.mode_valid = dw_dp_bridge_mode_valid,
	.atomic_check = dw_dp_bridge_atomic_check,
	.atomic_pre_enable = dw_dp_bridge_atomic_pre_enable,
	.atomic_enable = dw_dp_bridge_atomic_enable,
	.atomic_disable = dw_dp_bridge_atomic_disable,
	.detect = dw_dp_bridge_detect,
	.get_edid = dw_dp_bridge_get_edid,
};

static int dw_dp_link_retrain(struct dw_dp *dp)
{
	struct drm_device *dev = dp->bridge.dev;
	struct drm_modeset_acquire_ctx ctx;
	int ret;

	if (!dw_dp_needs_link_retrain(dp))
		return 0;

	dev_dbg(dp->dev, "Retraining link\n");

	drm_modeset_acquire_init(&ctx, 0);
	for (;;) {
		ret = drm_modeset_lock(&dev->mode_config.connection_mutex, &ctx);
		if (ret != -EDEADLK)
			break;

		drm_modeset_backoff(&ctx);
	}

	ret = dw_dp_link_train(dp);
	drm_modeset_drop_locks(&ctx);
	drm_modeset_acquire_fini(&ctx);

	return ret;
}

static void dw_dp_hpd_work(struct work_struct *work)
{
	struct dw_dp *dp = container_of(work, struct dw_dp, hpd_work);
	bool long_hpd;
	int ret;

	mutex_lock(&dp->irq_lock);
	long_hpd = dp->hotplug.long_hpd;
	mutex_unlock(&dp->irq_lock);

	dev_dbg(dp->dev, "got hpd irq - %s\n", long_hpd ? "long" : "short");

	if (!long_hpd) {
		ret = dw_dp_link_retrain(dp);
		if (ret)
			dev_warn(dp->dev, "Retrain link failed\n");
	} else {
		drm_helper_hpd_irq_event(dp->bridge.dev);
	}
}

static void dw_dp_handle_hpd_event(struct dw_dp *dp)
{
	u32 value;

	mutex_lock(&dp->irq_lock);

	regmap_read(dp->regmap, DPTX_HPD_STATUS, &value);

	if (value & HPD_IRQ) {
		dev_dbg(dp->dev, "IRQ from the HPD\n");
		dp->hotplug.long_hpd = false;
		regmap_write(dp->regmap, DPTX_HPD_STATUS, HPD_IRQ);
	}

	if (value & HPD_HOT_PLUG) {
		dev_dbg(dp->dev, "Hot plug detected\n");
		dp->hotplug.long_hpd = true;
		regmap_write(dp->regmap, DPTX_HPD_STATUS, HPD_HOT_PLUG);
	}

	if (value & HPD_HOT_UNPLUG) {
		dev_dbg(dp->dev, "Unplug detected\n");
		dp->hotplug.long_hpd = true;
		regmap_write(dp->regmap, DPTX_HPD_STATUS, HPD_HOT_UNPLUG);
	}

	mutex_unlock(&dp->irq_lock);

	schedule_work(&dp->hpd_work);
}

static irqreturn_t dw_dp_irq_handler(int irq, void *data)
{
	struct dw_dp *dp = data;
	u32 value;

	regmap_read(dp->regmap, DPTX_GENERAL_INTERRUPT, &value);
	if (!value)
		return IRQ_NONE;

	if (value & HPD_EVENT)
		dw_dp_handle_hpd_event(dp);

	if (value & AUX_REPLY_EVENT) {
		regmap_write(dp->regmap, DPTX_GENERAL_INTERRUPT,
			     AUX_REPLY_EVENT);
		complete(&dp->complete);
	}

	return IRQ_HANDLED;
}

static int dw_dp_audio_hw_params(struct device *dev, void *data,
				 struct hdmi_codec_daifmt *daifmt,
				 struct hdmi_codec_params *params)
{
	struct dw_dp *dp = dev_get_drvdata(dev);
	struct dw_dp_audio *audio = &dp->audio;
	u8 audio_data_in_en, num_channels, audio_inf_select;

	audio->channels = params->cea.channels;

	switch (params->cea.channels) {
	case 1:
		audio_data_in_en = 0x1;
		num_channels = 0x0;
		break;
	case 2:
		audio_data_in_en = 0x1;
		num_channels = 0x1;
		break;
	case 8:
		audio_data_in_en = 0xf;
		num_channels = 0x7;
		break;
	default:
		dev_err(dp->dev, "invalid channels %d\n", params->cea.channels);
		return -EINVAL;
	}

	switch (daifmt->fmt) {
	case HDMI_SPDIF:
		audio_inf_select = 0x1;
		break;
	case HDMI_I2S:
		audio_inf_select = 0x0;
		break;
	default:
		dev_err(dp->dev, "invalid daifmt %d\n", daifmt->fmt);
		return -EINVAL;
	}

	regmap_update_bits(dp->regmap, DPTX_AUD_CONFIG1,
			   AUDIO_DATA_IN_EN | NUM_CHANNELS | AUDIO_DATA_WIDTH |
			   AUDIO_INF_SELECT,
			   FIELD_PREP(AUDIO_DATA_IN_EN, audio_data_in_en) |
			   FIELD_PREP(NUM_CHANNELS, num_channels) |
			   FIELD_PREP(AUDIO_DATA_WIDTH, params->sample_width) |
			   FIELD_PREP(AUDIO_INF_SELECT, audio_inf_select));

	return 0;
}

static int dw_dp_audio_infoframe_send(struct dw_dp *dp)
{
	struct dw_dp_audio *audio = &dp->audio;
	struct hdmi_audio_infoframe frame;
	struct dp_sdp_header header;
	u8 buffer[HDMI_INFOFRAME_HEADER_SIZE + HDMI_DRM_INFOFRAME_SIZE];
	u8 size = sizeof(buffer);
	int i, j, ret;

	header.HB0 = 0;
	header.HB1 = HDMI_INFOFRAME_TYPE_AUDIO;
	header.HB2 = 0x1b;
	header.HB3 = 0x48;

	ret = hdmi_audio_infoframe_init(&frame);
	if (ret < 0)
		return ret;

	frame.coding_type = HDMI_AUDIO_CODING_TYPE_STREAM;
	frame.sample_frequency = HDMI_AUDIO_SAMPLE_FREQUENCY_STREAM;
	frame.sample_size = HDMI_AUDIO_SAMPLE_SIZE_STREAM;
	frame.channels = audio->channels;

	ret = hdmi_audio_infoframe_pack(&frame, buffer, sizeof(buffer));
	if (ret < 0)
		return ret;

	regmap_write(dp->regmap, DPTX_SDP_REGISTER_BANK,
		     get_unaligned_le32(&header));

	for (i = 1; i < DIV_ROUND_UP(size, 4); i++) {
		size_t num = min_t(size_t, size - i * 4, 4);
		u32 value = 0;

		for (j = 0; j < num; j++)
			value |= buffer[i * 4 + j] << (j * 8);

		regmap_write(dp->regmap, DPTX_SDP_REGISTER_BANK + 4 * i, value);
	}

	regmap_update_bits(dp->regmap, DPTX_SDP_VERTICAL_CTRL,
			   EN_VERTICAL_SDP, FIELD_PREP(EN_VERTICAL_SDP, 1));

	return 0;
}

static int dw_dp_audio_startup(struct device *dev, void *data)
{
	struct dw_dp *dp = dev_get_drvdata(dev);

	regmap_update_bits(dp->regmap, DPTX_SDP_VERTICAL_CTRL,
			   EN_AUDIO_STREAM_SDP | EN_AUDIO_TIMESTAMP_SDP,
			   FIELD_PREP(EN_AUDIO_STREAM_SDP, 1) |
			   FIELD_PREP(EN_AUDIO_TIMESTAMP_SDP, 1));
	regmap_update_bits(dp->regmap, DPTX_SDP_HORIZONTAL_CTRL,
			   EN_AUDIO_STREAM_SDP,
			   FIELD_PREP(EN_AUDIO_STREAM_SDP, 1));

	return dw_dp_audio_infoframe_send(dp);
}

static void dw_dp_audio_shutdown(struct device *dev, void *data)
{
	struct dw_dp *dp = dev_get_drvdata(dev);

	regmap_update_bits(dp->regmap, DPTX_AUD_CONFIG1, AUDIO_DATA_IN_EN,
			   FIELD_PREP(AUDIO_DATA_IN_EN, 0));
}

static int dw_dp_audio_get_eld(struct device *dev, void *data, uint8_t *buf,
			       size_t len)
{
	struct dw_dp *dp = dev_get_drvdata(dev);
	struct drm_connector *connector = &dp->connector;

	memcpy(buf, connector->eld, min(sizeof(connector->eld), len));

	return 0;
}

static const struct hdmi_codec_ops dw_dp_audio_codec_ops = {
	.hw_params = dw_dp_audio_hw_params,
	.audio_startup = dw_dp_audio_startup,
	.audio_shutdown = dw_dp_audio_shutdown,
	.get_eld = dw_dp_audio_get_eld,
};

static int dw_dp_register_audio_driver(struct dw_dp *dp)
{
	struct dw_dp_audio *audio = &dp->audio;
	struct hdmi_codec_pdata codec_data = {
		.ops = &dw_dp_audio_codec_ops,
		.spdif = 1,
		.i2s = 1,
		.max_i2s_channels = 8,
	};

	audio->pdev = platform_device_register_data(dp->dev,
						    HDMI_CODEC_DRV_NAME,
						    PLATFORM_DEVID_AUTO,
						    &codec_data,
						    sizeof(codec_data));

	return PTR_ERR_OR_ZERO(audio->pdev);
}

static void dw_dp_unregister_audio_driver(void *data)
{
	struct dw_dp *dp = data;
	struct dw_dp_audio *audio = &dp->audio;

	if (audio->pdev) {
		platform_device_unregister(audio->pdev);
		audio->pdev = NULL;
	}
}

static void dw_dp_aux_unregister(void *data)
{
	struct dw_dp *dp = data;

	drm_dp_aux_unregister(&dp->aux);
}

static int dw_dp_bind(struct device *dev, struct device *master, void *data)
{
	struct dw_dp *dp = dev_get_drvdata(dev);
	struct drm_device *drm_dev = data;
	struct drm_encoder *encoder = &dp->encoder;
	struct drm_bridge *bridge = &dp->bridge;
	int ret;

	if (!dp->left) {
		drm_simple_encoder_init(drm_dev, encoder, DRM_MODE_ENCODER_TMDS);
		drm_encoder_helper_add(encoder, &dw_dp_encoder_helper_funcs);

		encoder->possible_crtcs =
			rockchip_drm_of_find_possible_crtcs(drm_dev, dev->of_node);

		ret = drm_bridge_attach(encoder, bridge, NULL, 0);
		if (ret) {
			dev_err(dev, "failed to attach bridge: %d\n", ret);
			return ret;
		}
	}

	if (dp->right) {
		struct dw_dp *secondary = dp->right;

		ret = drm_bridge_attach(encoder, &secondary->bridge, bridge,
					DRM_BRIDGE_ATTACH_NO_CONNECTOR);
		if (ret)
			return ret;
	}

	pm_runtime_enable(dp->dev);
	pm_runtime_get_sync(dp->dev);

	return 0;
}

static void dw_dp_unbind(struct device *dev, struct device *master, void *data)
{
	struct dw_dp *dp = dev_get_drvdata(dev);

	pm_runtime_put(dp->dev);
	pm_runtime_disable(dp->dev);

	drm_encoder_cleanup(&dp->encoder);
}

static const struct component_ops dw_dp_component_ops = {
	.bind = dw_dp_bind,
	.unbind = dw_dp_unbind,
};

static const struct regmap_range dw_dp_readable_ranges[] = {
	regmap_reg_range(DPTX_VERSION_NUMBER, DPTX_ID),
	regmap_reg_range(DPTX_CONFIG_REG1, DPTX_CONFIG_REG3),
	regmap_reg_range(DPTX_CCTL, DPTX_SOFT_RESET_CTRL),
	regmap_reg_range(DPTX_VSAMPLE_CTRL, DPTX_VIDEO_HBLANK_INTERVAL),
	regmap_reg_range(DPTX_AUD_CONFIG1, DPTX_AUD_CONFIG1),
	regmap_reg_range(DPTX_SDP_VERTICAL_CTRL, DPTX_SDP_STATUS_EN),
	regmap_reg_range(DPTX_PHYIF_CTRL, DPTX_PHYIF_PWRDOWN_CTRL),
	regmap_reg_range(DPTX_AUX_CMD, DPTX_AUX_DATA3),
	regmap_reg_range(DPTX_GENERAL_INTERRUPT, DPTX_HPD_INTERRUPT_ENABLE),
};

static const struct regmap_access_table dw_dp_readable_table = {
	.yes_ranges     = dw_dp_readable_ranges,
	.n_yes_ranges   = ARRAY_SIZE(dw_dp_readable_ranges),
};

static const struct regmap_config dw_dp_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.fast_io = true,
	.max_register = DPTX_MAX_REGISTER,
	.rd_table = &dw_dp_readable_table,
};

static int dw_dp_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct dw_dp *dp;
	void __iomem *base;
	int id, ret;

	dp = devm_kzalloc(dev, sizeof(*dp), GFP_KERNEL);
	if (!dp)
		return -ENOMEM;

	id = of_alias_get_id(dev->of_node, "dp");
	if (id < 0)
		id = 0;

	dp->id = id;
	dp->dev = dev;
	dp->video.pixel_mode = DPTX_MP_QUAD_PIXEL;

	mutex_init(&dp->irq_lock);
	INIT_WORK(&dp->hpd_work, dw_dp_hpd_work);
	init_completion(&dp->complete);

	base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(base))
		return PTR_ERR(base);

	dp->regmap = devm_regmap_init_mmio(dev, base, &dw_dp_regmap_config);
	if (IS_ERR(dp->regmap))
		return dev_err_probe(dev, PTR_ERR(dp->regmap),
				     "failed to create regmap\n");

	dp->phy = devm_of_phy_get(dev, dev->of_node, NULL);
	if (IS_ERR(dp->phy))
		return dev_err_probe(dev, PTR_ERR(dp->phy),
				     "failed to get phy\n");

	ret = devm_clk_bulk_get_all(dev, &dp->clks);
	if (ret < 1)
		return dev_err_probe(dev, ret, "failed to get clocks\n");

	dp->nr_clks = ret;

	dp->rstc = devm_reset_control_get(dev, NULL);
	if (IS_ERR(dp->rstc))
		return dev_err_probe(dev, PTR_ERR(dp->rstc),
				     "failed to get reset control\n");

	dp->hpd_gpio = devm_gpiod_get_optional(dev, "hpd", GPIOD_IN);
	if (IS_ERR(dp->hpd_gpio))
		return dev_err_probe(dev, PTR_ERR(dp->hpd_gpio),
				     "failed to get hpd GPIO\n");
	if (dp->hpd_gpio) {
		int hpd_irq = gpiod_to_irq(dp->hpd_gpio);

		ret = devm_request_threaded_irq(dev, hpd_irq, NULL,
						dw_dp_hpd_irq_handler,
						IRQF_TRIGGER_RISING |
						IRQF_TRIGGER_FALLING |
						IRQF_ONESHOT, "dw-dp-hpd", dp);
		if (ret) {
			dev_err(dev, "failed to request HPD interrupt\n");
			return ret;
		}
	}

	dp->irq = platform_get_irq(pdev, 0);
	if (dp->irq < 0)
		return dp->irq;

	irq_set_status_flags(dp->irq, IRQ_NOAUTOEN);
	ret = devm_request_threaded_irq(dev, dp->irq, NULL, dw_dp_irq_handler,
					IRQF_ONESHOT, dev_name(dev), dp);
	if (ret) {
		dev_err(dev, "failed to request irq: %d\n", ret);
		return ret;
	}

	dp->extcon = devm_extcon_dev_allocate(dev, dw_dp_cable);
	if (IS_ERR(dp->extcon))
		return dev_err_probe(dev, PTR_ERR(dp->extcon),
				     "failed to allocate extcon device\n");

	ret = devm_extcon_dev_register(dev, dp->extcon);
	if (ret)
		return dev_err_probe(dev, ret,
				     "failed to register extcon device\n");

	ret = dw_dp_register_audio_driver(dp);
	if (ret)
		return ret;

	ret = devm_add_action_or_reset(dev, dw_dp_unregister_audio_driver, dp);
	if (ret)
		return ret;

	dp->aux.dev = dev;
	dp->aux.name = dev_name(dev);
	dp->aux.transfer = dw_dp_aux_transfer;
	ret = drm_dp_aux_register(&dp->aux);
	if (ret)
		return ret;

	ret = devm_add_action_or_reset(dev, dw_dp_aux_unregister, dp);
	if (ret)
		return ret;

	dp->bridge.of_node = dev->of_node;
	dp->bridge.funcs = &dw_dp_bridge_funcs;
	dp->bridge.ops = DRM_BRIDGE_OP_DETECT | DRM_BRIDGE_OP_EDID |
			 DRM_BRIDGE_OP_HPD;
	dp->bridge.type = DRM_MODE_CONNECTOR_DisplayPort;

	platform_set_drvdata(pdev, dp);

	if (device_property_read_bool(dev, "split-mode")) {
		struct dw_dp *secondary = dw_dp_find_by_id(dev->driver, !dp->id);

		if (!secondary)
			return -EPROBE_DEFER;

		dp->right = secondary;
		dp->split_mode = true;
		secondary->left = dp;
		secondary->split_mode = true;
	}

	return component_add(dev, &dw_dp_component_ops);
}

static int dw_dp_remove(struct platform_device *pdev)
{
	struct dw_dp *dp = platform_get_drvdata(pdev);

	component_del(dp->dev, &dw_dp_component_ops);
	cancel_work_sync(&dp->hpd_work);

	return 0;
}

static int __maybe_unused dw_dp_runtime_suspend(struct device *dev)
{
	struct dw_dp *dp = dev_get_drvdata(dev);

	disable_irq(dp->irq);

	clk_bulk_disable_unprepare(dp->nr_clks, dp->clks);

	return 0;
}

static int __maybe_unused dw_dp_runtime_resume(struct device *dev)
{
	struct dw_dp *dp = dev_get_drvdata(dev);
	int ret;

	ret = clk_bulk_prepare_enable(dp->nr_clks, dp->clks);
	if (ret)
		return ret;

	reset_control_assert(dp->rstc);
	udelay(10);
	reset_control_deassert(dp->rstc);

	dw_dp_init(dp);
	enable_irq(dp->irq);

	return 0;
}

static const struct dev_pm_ops dw_dp_pm_ops = {
	SET_RUNTIME_PM_OPS(dw_dp_runtime_suspend, dw_dp_runtime_resume, NULL)
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
};

static const struct of_device_id dw_dp_of_match[] = {
	{ .compatible = "rockchip,rk3588-dp", },
	{}
};
MODULE_DEVICE_TABLE(of, dw_dp_of_match);

struct platform_driver dw_dp_driver = {
	.probe	= dw_dp_probe,
	.remove = dw_dp_remove,
	.driver = {
		.name = "dw-dp",
		.of_match_table = dw_dp_of_match,
		.pm = &dw_dp_pm_ops,
	},
};
