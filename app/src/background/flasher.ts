import { listDevices,   } from 'nodemcu-tool';
import path from 'path';
import fs from 'fs';
import electron from 'electron';
import glob from 'glob';
import https from 'https';
import { exec } from 'child_process';

// list of known vendor IDs
const supportedBoards = [
    // NodeMCU v3 - CH340G Adapter | 0x1A86 Nanjing QinHeng Electronics Co., Ltd.
    '1A86'
];

class Flasher {
    async getSupportedDevices() {
        const ports = await listDevices(true);
        return ports.filter(function(item){
            return supportedBoards.includes(item.vendorId && item.vendorId.toUpperCase());
        });
    }

    getFirmwareSavePath() {
        const userDataPath = (electron.app || electron.remote.app).getPath('userData');
        return path.join(userDataPath, 'Firmwares');
    }

    async getFirmwares() {
        const firmwarePath = this.getFirmwareSavePath();
        const files = glob.sync(firmwarePath + '/*.bin', {});
        const fileNames = [];
        files.forEach((file) => {
            fileNames.push(path.parse(file).name);
        })
        return fileNames;
    }

    async flashNode(port, firmware) {
        const filename = path.join(this.getFirmwareSavePath(), firmware.version + ".bin");
        const cmd = 'arduino-cli upload -b "esp8266:esp8266:nodemcuv2" --input "' + filename + '" -p ' + port.path;
        exec(cmd, (error, stdout, stderr) => {
            if (error) {
                console.log(`error: ${error.message}`);
                return;
            }
            if (stderr) {
                console.log(`stderr: ${stderr}`);
                return;
            }
            console.log(`stdout: ${stdout}`);
        });
    }

    /**
     * Download a resource from `url` to `dest`.
     * @param {string} url - Valid URL to attempt download of resource
     * @param {string} dest - Valid path to save the file.
     * @returns {Promise<void>} - Returns asynchronously when successfully completed download
     */
     download(url, dest) {
        return new Promise((resolve, reject) => {
            const request = https.get(url, response => {
                if (response.statusCode === 200) {

                    const file = fs.createWriteStream(dest, { flags: 'wx' });
                    file.on('finish', () => resolve());
                    file.on('error', err => {
                        file.close();
                        if (err.code === 'EEXIST') reject('File already exists');
                        else fs.unlink(dest, () => reject(err.message)); // Delete temp file
                    });
                    response.pipe(file);
                } else if (response.statusCode === 302 || response.statusCode === 301) {
                    //Recursively follow redirects, only a 200 will resolve.
                    this.download(response.headers.location, dest).then(() => resolve());
                } else {
                    reject(`Server responded with ${response.statusCode}: ${response.statusMessage}`);
                }
            });

            request.on('error', err => {
                reject(err.message);
            });
        });
    }

    async downloadFirmware(firmware) {
        const firmwareSavePath = this.getFirmwareSavePath();
        if (!fs.existsSync(firmwareSavePath)){
            fs.mkdirSync(firmwareSavePath);
        }
        const filename = path.join(this.getFirmwareSavePath(), firmware.version + ".bin");
        return this.download(firmware.downloadUrl, filename).then(() => {
            return this.getFirmwares();
        });
    }
}

export { Flasher };