# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct /home/sebas/Escritorio/proyecto/workspace/adc_dma_test/vitis_workspace/adc_dma_bd_wrapper/platform.tcl
# 
# OR launch xsct and run below command.
# source /home/sebas/Escritorio/proyecto/workspace/adc_dma_test/vitis_workspace/adc_dma_bd_wrapper/platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {adc_dma_bd_wrapper}\
-hw {/home/sebas/Escritorio/proyecto/workspace/adc_dma_test/adc_dma_bd_wrapper.xsa}\
-out {/home/sebas/Escritorio/proyecto/workspace/adc_dma_test/vitis_workspace}

platform write
domain create -name {standalone_ps7_cortexa9_0} -display-name {standalone_ps7_cortexa9_0} -os {standalone} -proc {ps7_cortexa9_0} -runtime {cpp} -arch {32-bit} -support-app {empty_application}
platform generate -domains 
platform active {adc_dma_bd_wrapper}
domain active {zynq_fsbl}
domain active {standalone_ps7_cortexa9_0}
platform generate -quick
platform generate
platform generate
platform generate
platform clean
platform generate
platform active {adc_dma_bd_wrapper}
platform config -updatehw {/home/sebas/Escritorio/proyecto/workspace/adc_dma_test/adc_dma_bd_wrapper.xsa}
platform generate -domains 
platform active {adc_dma_bd_wrapper}
platform config -updatehw {/home/sebas/Escritorio/proyecto/workspace/adc_dma_test/adc_dma_bd_wrapper.xsa}
platform active {adc_dma_bd_wrapper}
platform config -updatehw {/home/sebas/Escritorio/proyecto/workspace/adc_dma_test/adc_dma_bd_wrapper.xsa}
platform config -updatehw {/home/sebas/Escritorio/proyecto/workspace/adc_dma_test/adc_dma_bd_wrapper.xsa}
platform generate
platform generate -domains standalone_ps7_cortexa9_0 
platform clean
platform clean
platform config -updatehw {/home/sebas/Escritorio/proyecto/workspace/adc_dma_test/adc_dma_bd_wrapper.xsa}
platform generate
platform generate
platform active {adc_dma_bd_wrapper}
platform generate -domains 
platform active {adc_dma_bd_wrapper}
platform config -updatehw {/home/sebas/Escritorio/proyecto/workspace/adc_dma_test/adc_dma_bd_wrapper.xsa}
platform generate
platform active {adc_dma_bd_wrapper}
platform config -updatehw {/home/sebas/Escritorio/proyecto/workspace/adc_dma_test/adc_dma_bd_wrapper.xsa}
platform generate -domains 
platform active {adc_dma_bd_wrapper}
platform config -updatehw {/home/sebas/Escritorio/proyecto/workspace/adc_dma_test/adc_dma_bd_wrapper.xsa}
platform generate -domains 
platform clean
platform clean
platform generate
platform clean
platform clean
platform generate
platform generate
platform generate -domains standalone_ps7_cortexa9_0,zynq_fsbl 
platform active {adc_dma_bd_wrapper}
platform config -updatehw {/home/sebas/Escritorio/proyecto/workspace/adc_dma_test/adc_dma_bd_wrapper.xsa}
platform generate -domains 
platform config -updatehw {/home/sebas/Escritorio/proyecto/workspace/adc_dma_test/adc_dma_bd_wrapper.xsa}
platform clean
platform clean
platform generate
platform active {adc_dma_bd_wrapper}
platform config -updatehw {/home/sebas/Escritorio/proyecto/workspace/adc_dma_test/adc_dma_bd_wrapper.xsa}
platform generate -domains standalone_ps7_cortexa9_0,zynq_fsbl 
platform config -updatehw {/home/sebas/Escritorio/proyecto/workspace/adc_dma_test/adc_dma_bd_wrapper.xsa}
platform generate -domains 
platform clean
platform clean
platform generate
platform clean
platform clean
platform generate
platform clean
platform generate
platform generate -domains standalone_ps7_cortexa9_0,zynq_fsbl 
platform active {adc_dma_bd_wrapper}
platform config -updatehw {/home/sebas/Escritorio/proyecto/workspace/adc_dma_test/adc_dma_bd_wrapper.xsa}
platform clean
platform clean
platform generate
platform generate
platform active {adc_dma_bd_wrapper}
platform config -updatehw {/home/sebas/Escritorio/proyecto/workspace/adc_dma_test/adc_dma_bd_wrapper.xsa}
platform generate -domains 
platform config -updatehw {/home/sebas/Escritorio/proyecto/workspace/adc_dma_test/adc_dma_bd_wrapper.xsa}
platform generate -domains 
platform clean
platform generate
platform active {adc_dma_bd_wrapper}
platform config -updatehw {/home/sebas/Escritorio/proyecto/workspace/adc_dma_test/adc_dma_bd_wrapper.xsa}
platform clean
platform generate
platform active {adc_dma_bd_wrapper}
bsp reload
bsp config stdin "ps7_uart_0"
bsp reload
bsp config stdin "ps7_uart_0"
bsp config stdout "ps7_uart_0"
bsp write
platform generate
platform active {adc_dma_bd_wrapper}
platform config -updatehw {/home/sebas/Escritorio/proyecto/workspace/adc_dma_test/adc_dma_bd_wrapper.xsa}
platform clean
platform clean
platform generate
platform generate
platform clean
platform clean
platform generate
platform clean
platform generate
platform generate -domains standalone_ps7_cortexa9_0 
platform generate -domains standalone_ps7_cortexa9_0,zynq_fsbl 
platform active {adc_dma_bd_wrapper}
platform config -updatehw {/home/sebas/Escritorio/proyecto/workspace/adc_dma_test/adc_dma_bd_wrapper.xsa}
platform generate -domains 
platform generate -domains standalone_ps7_cortexa9_0,zynq_fsbl 
platform generate -domains standalone_ps7_cortexa9_0,zynq_fsbl 
platform generate -domains standalone_ps7_cortexa9_0,zynq_fsbl 
platform generate -domains 
platform generate -domains standalone_ps7_cortexa9_0,zynq_fsbl 
platform generate
platform active {adc_dma_bd_wrapper}
bsp reload
bsp setlib -name xilffs -ver 5.0
bsp reload
bsp setlib -name xilffs -ver 5.0
bsp write
bsp reload
catch {bsp regenerate}
platform generate -domains standalone_ps7_cortexa9_0 
platform clean
platform generate
platform clean
platform generate
