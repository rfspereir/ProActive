const app = import(`./dist/proactive/server/server.mjs`).then(server => server.app());
exports.handle = (req,res) => app.then(it => it(req,res));
