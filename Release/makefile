################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include LCD/subdir.mk
-include 1Wire/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
LSS += \
Wytrawiarka.lss \

FLASH_IMAGE += \
Wytrawiarka.hex \

EEPROM_IMAGE += \
Wytrawiarka.eep \

SIZEDUMMY += \
sizedummy \


# All Target
all: Wytrawiarka.elf secondary-outputs

# Tool invocations
Wytrawiarka.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: AVR C Linker'
	avr-gcc -Wl,-Map,Wytrawiarka.map -mmcu=atmega8 -o "Wytrawiarka.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

Wytrawiarka.lss: Wytrawiarka.elf
	@echo 'Invoking: AVR Create Extended Listing'
	-avr-objdump -h -S Wytrawiarka.elf  >"Wytrawiarka.lss"
	@echo 'Finished building: $@'
	@echo ' '

Wytrawiarka.hex: Wytrawiarka.elf
	@echo 'Create Flash image (ihex format)'
	-avr-objcopy -R .eeprom -R .fuse -R .lock -R .signature -O ihex Wytrawiarka.elf  "Wytrawiarka.hex"
	@echo 'Finished building: $@'
	@echo ' '

Wytrawiarka.eep: Wytrawiarka.elf
	@echo 'Create eeprom image (ihex format)'
	-avr-objcopy -j .eeprom --no-change-warnings --change-section-lma .eeprom=0 -O ihex Wytrawiarka.elf  "Wytrawiarka.eep"
	@echo 'Finished building: $@'
	@echo ' '

sizedummy: Wytrawiarka.elf
	@echo 'Invoking: Print Size'
	-avr-size --format=avr --mcu=atmega8 Wytrawiarka.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(FLASH_IMAGE)$(ELFS)$(OBJS)$(ASM_DEPS)$(EEPROM_IMAGE)$(S_DEPS)$(SIZEDUMMY)$(S_UPPER_DEPS)$(LSS)$(C_DEPS) Wytrawiarka.elf
	-@echo ' '

secondary-outputs: $(LSS) $(FLASH_IMAGE) $(EEPROM_IMAGE) $(SIZEDUMMY)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
