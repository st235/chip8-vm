"use strict";

const canvas = document.getElementById("canvas");
const upload = document.getElementById("upload");

async function loadGame(nativeLoadGame, e) {
    const file = e.target.files[0];
    const data = new Uint8Array(await file.arrayBuffer());
    const filename = "/rom.ch8";
    FS.writeFile(filename, data);
    nativeLoadGame(filename);
}

var Module = {
    canvas,
    print: (...args) => console.log(...args),
    onRuntimeInitialized() {
        console.log("On runtime initialized.");
        const nativeLoadGame = Module.cwrap("loadGame", null, ["string"]);
        upload.onchange = (e) => loadGame(nativeLoadGame, e);
    },
};
