var pkg = require('./package.json');

console.log(
    pkg.binary.host +
    'v' + pkg.version + '/' +
    pkg.binary.module_name + '-' +
    'v' + pkg.version + '-' +
    (process.env.NODE_BIN_RUNTIME || 'node') + '-' +
    'v' + (process.env.NODE_BIN_ABI || process.versions.modules) + '-' +
    (process.env.NODE_BIN_TARGET_PLATFORM || process.platform) + '-' +
    (process.env.NODE_BIN_TARGET_ARCH || process.arch) + '.tar.gz'
);
