import * as m from 'zigbee-herdsman-converters/lib/modernExtend';

// Keep MQTT properties and endpoints stable; customize presentation only.
const relayControls = m.onOff({endpointNames: ['power', 'backlight', 'max_power'], powerOnBehavior: false});
const relayPresentation = {
    power: {
        label: 'Power',
        description: 'Turn humidification on or off',
        icon: 'mdi:power',
    },
    backlight: {
        label: 'Backlight Dim',
        description: 'ON: dim backlight (night mode). OFF: normal backlight brightness.',
        icon: 'mdi:brightness-4',
    },
    max_power: {
        label: 'Max Power',
        description: 'ON: maximum mist output regardless of the output control knob position. OFF: mist output follows the control knob.',
        icon: 'mdi:fan-speed-3',
    },
};
for (const expose of relayControls.exposes) {
    const {label, description, icon} = relayPresentation[expose.endpoint];
    expose.withHomeAssistant({name: label, icon});
    expose.features.find((feature) => feature.name === 'state')
        .withLabel(label)
        .withDescription(description);
}

export default {
    zigbeeModel: ['HumidifierKit-H2'],
    model: 'HumidifierKit-H2',
    vendor: 'Orest',
    ota: {manufacturerName: 'Orest', modelId: 'HumidifierKit-H2'},
    description: 'Three independent relay outputs and water tank empty input',
    extend: [
        m.deviceEndpoints({endpoints: {power: 1, backlight: 2, max_power: 3, tank: 4}}),
        relayControls,
        m.binary({
            name: 'water_empty',
            cluster: 'genBinaryInput',
            attribute: 'presentValue',
            endpointName: 'tank',
            description: 'Water tank is empty (information only)',
            valueOn: [true, 1],
            valueOff: [false, 0],
            access: 'STATE_GET',
            reporting: {min: 0, max: 60, change: 1},
        }),
    ],
};
