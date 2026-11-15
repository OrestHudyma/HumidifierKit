import * as m from 'zigbee-herdsman-converters/lib/modernExtend';

export default {
    zigbeeModel: ['HumidifierKit-H2'],
    model: 'HumidifierKit-H2',
    vendor: 'Orest',
    ota: {manufacturerName: 'Orest', modelId: 'HumidifierKit-H2'},
    description: 'Three independent relay outputs and water tank empty input',
    extend: [
        m.deviceEndpoints({endpoints: {power: 1, backlight: 2, max_power: 3, tank: 4}}),
        m.onOff({endpointNames: ['power', 'backlight', 'max_power'], powerOnBehavior: false}),
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
