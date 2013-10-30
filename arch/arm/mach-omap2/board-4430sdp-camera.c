#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/i2c/twl.h>
#include <linux/mfd/twl6040.h>
#include <linux/regulator/consumer.h>

#include <plat/i2c.h>
#include <plat/omap-pm.h>

#include <asm/mach-types.h>

#include <media/ov5650.h>

#include "devices.h"
#include "../../../drivers/media/video/omap4iss/iss.h"

#include "control.h"
#include "mux.h"

#define OMAP4430SDP_GPIO_CAM_PDN_B	38
#define OMAP4430SDP_GPIO_CAM_PDN_C	39

/* TWL6040.GPOCTL[2] GPO3 ? Hopefully... */
/* 509 is base for new twl6040 gpio driver, and + 2 is GPO3 */
#define OMAP4430SDP_GPIO_PCAM_SEL	(509 + 2)

/*
 * Use GPO3 pin from Phoenix Audio IC to control CAM_SEL in
 * 4430SDP Camera board
 */
static int sdp4430_ov_cam1_pre_poweron(struct v4l2_subdev *subdev)
{
	gpio_set_value_cansleep(OMAP4430SDP_GPIO_PCAM_SEL, 0);
	
	return 0;
}

static int sdp4430_ov_cam2_pre_poweron(struct v4l2_subdev *subdev)
{
	gpio_set_value_cansleep(OMAP4430SDP_GPIO_PCAM_SEL, 1);
	
	return 0;
}

#define OV5650_I2C_ADDRESS   (0x36)

static struct ov5650_platform_data ov5650_cam1_platform_data = {
	.reg_avdd = "cam2pwr",
	.reg_dovdd = NULL, /* Hardwired on */

	.clk_xvclk = "auxclk2_ck",

	.gpio_pwdn = OMAP4430SDP_GPIO_CAM_PDN_B,
	.is_gpio_pwdn_acthi = 1,
	.gpio_resetb = -1, /* Not connected */

	.pre_poweron = sdp4430_ov_cam1_pre_poweron,
	.post_poweroff = NULL,
};

static struct i2c_board_info ov5650_cam1_i2c_device = {
	I2C_BOARD_INFO("ov5650", OV5650_I2C_ADDRESS),
	.platform_data = &ov5650_cam1_platform_data,
};

static struct ov5650_platform_data ov5650_cam2_platform_data = {
	.reg_avdd = "cam2pwr",
	.reg_dovdd = NULL, /* Hardwired on */

	.clk_xvclk = "auxclk3_ck",

	.gpio_pwdn = OMAP4430SDP_GPIO_CAM_PDN_C,
	.is_gpio_pwdn_acthi = 1,
	.gpio_resetb = -1, /* Not connected */

	.pre_poweron = sdp4430_ov_cam2_pre_poweron,
	.post_poweroff = NULL,
};

static struct i2c_board_info ov5650_cam2_i2c_device = {
	I2C_BOARD_INFO("ov5650", OV5650_I2C_ADDRESS),
	.platform_data = &ov5650_cam2_platform_data,
};

static struct iss_subdev_i2c_board_info ov5650_cam1_subdevs[] = {
	{
		.board_info = &ov5650_cam1_i2c_device,
		.i2c_adapter_id = 2,
	},
	{ NULL, 0, },
};

static struct iss_subdev_i2c_board_info ov5650_cam2_subdevs[] = {
	{
		.board_info = &ov5650_cam2_i2c_device,
		.i2c_adapter_id = 3,
	},
	{ NULL, 0, },
};

static struct iss_v4l2_subdevs_group sdp4430_camera_subdevs[] = {
	{
		.subdevs = ov5650_cam1_subdevs,
		.interface = ISS_INTERFACE_CSI2B_PHY2,
		.bus = { .csi2 = {
			.lanecfg	= {
				.clk = {
					.pol = 0,
					.pos = 1,
				},
				.data[0] = {
					.pol = 0,
					.pos = 2,
				},
			},
		} },
	},
	{
		.subdevs = ov5650_cam2_subdevs,
		.interface = ISS_INTERFACE_CSI2A_PHY1,
		.bus = { .csi2 = {
			.lanecfg	= {
				.clk = {
					.pol = 0,
					.pos = 1,
				},
				.data[0] = {
					.pol = 0,
					.pos = 2,
				},
			},
		} },
	},
	{ },
};

static void sdp4430_omap4iss_set_constraints(struct iss_device *iss, bool enable)
{
	if (!iss)
		return;

	/* FIXME: Look for something more precise as a good throughtput limit */
	omap_pm_set_min_bus_tput(iss->dev, OCP_INITIATOR_AGENT,
				 enable ? 800000 : -1);
}

static struct iss_platform_data sdp4430_iss_platform_data = {
	.subdevs = sdp4430_camera_subdevs,
	.set_constraints = sdp4430_omap4iss_set_constraints,
};

static struct omap_device_pad omap4iss_pads[] = {
	/* CSI2-A */
	{
		.name   = "csi21_dx0.csi21_dx0",
		.enable = OMAP_MUX_MODE0 | OMAP_INPUT_EN,
	},
	{
		.name   = "csi21_dy0.csi21_dy0",
		.enable = OMAP_MUX_MODE0 | OMAP_INPUT_EN,
	},
	{
		.name   = "csi21_dx1.csi21_dx1",
		.enable = OMAP_MUX_MODE0 | OMAP_INPUT_EN,
	},
	{
		.name   = "csi21_dy1.csi21_dy1",
		.enable = OMAP_MUX_MODE0 | OMAP_INPUT_EN,
	},
	{
		.name   = "csi21_dx2.csi21_dx2",
		.enable = OMAP_MUX_MODE0 | OMAP_INPUT_EN,
	},
	{
		.name   = "csi21_dy2.csi21_dy2",
		.enable = OMAP_MUX_MODE0 | OMAP_INPUT_EN,
	},
	/* CSI2-B */
	{
		.name   = "csi22_dx0.csi22_dx0",
		.enable = OMAP_MUX_MODE0 | OMAP_INPUT_EN,
	},
	{
		.name   = "csi22_dy0.csi22_dy0",
		.enable = OMAP_MUX_MODE0 | OMAP_INPUT_EN,
	},
	{
		.name   = "csi22_dx1.csi22_dx1",
		.enable = OMAP_MUX_MODE0 | OMAP_INPUT_EN,
	},
	{
		.name   = "csi22_dy1.csi22_dy1",
		.enable = OMAP_MUX_MODE0 | OMAP_INPUT_EN,
	},
};

static struct omap_board_data omap4iss_data = {
	.id	    		= 1,
	.pads	 		= omap4iss_pads,
	.pads_cnt       	= ARRAY_SIZE(omap4iss_pads),
};

static int __init sdp4430_camera_init(void)
{
	if (!machine_is_omap_4430sdp())
		return 0;

	omap_mux_init_gpio(OMAP4430SDP_GPIO_CAM_PDN_B, OMAP_PIN_OUTPUT);
	omap_mux_init_gpio(OMAP4430SDP_GPIO_CAM_PDN_C, OMAP_PIN_OUTPUT);

	/* Init FREF_CLK2_OUT */
	omap_mux_init_signal("fref_clk2_out", OMAP_PIN_OUTPUT);

	/* Init FREF_CLK3_OUT */
	omap_mux_init_signal("fref_clk3_out", OMAP_PIN_OUTPUT);

	if (gpio_request_one(OMAP4430SDP_GPIO_PCAM_SEL,
			     GPIOF_OUT_INIT_LOW,
			     "PCAM_SEL")) {
		printk(KERN_ERR "Unable to request PCAM_SEL gpio\n");

		return -ENODEV;
	}	
	
	return omap4_init_camera(&sdp4430_iss_platform_data, &omap4iss_data);
}
late_initcall(sdp4430_camera_init);
