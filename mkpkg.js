var fs = require('fs')
var pkg = require('./package.json');

var dist = pkg.binary.module_path;

try {
    fs.statSync(dist);
}
catch (err) {
    fs.mkdirSync(dist);
}

fs.writeFileSync(dist + '/package.json',
    JSON.stringify({
        name: pkg.name + '-bin',
        description: pkg.description + ' (Precompiled)',
        version: pkg.version,
        private: true,
        license: pkg.license,
        author:  pkg.author,
        repository: pkg.repository,
        homepage: pkg.homepage,
        main: pkg.binary.module_name + '.node'
    })
);
