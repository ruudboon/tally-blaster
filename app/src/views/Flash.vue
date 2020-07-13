<template>
  <v-main>
    <v-container fill-height>
      <v-layout row align-center>
        <v-col>
          <v-img
                  alt="Tally Blaster Logo"
                  contain
                  src="@/assets/logo.svg"
          />
        </v-col>
        <v-col>
          <v-alert type="error">
            This part is highly experimental. We use arduino-cli to flash the device while we work on implementing a clean solution.
            Make sure arduino-cli is installed and the board (esp8266:esp8266:nodemcuv2) is added.
          </v-alert>
          <v-form ref="form">
            <v-select
                    v-model="selectedPort"
                    :items="ports"
                    label="Device"
                    item-text="path"
                    required
                    no-data-text="No device detected"
            >
            </v-select>
            <v-select
                    v-model="selectedFirmware"
                    :items="firmwares"
                    label="Firmware"
                    required
                    no-data-text="No firmware found"
            >
              <template slot="selection" slot-scope="data">
                <span>{{ data.item.version }}</span>
                <span v-if="!data.item.stable">| Pre-release </span>
                <span v-if="data.item.downloaded">| downloaded</span>
                <span v-if="!data.item.downloaded">| not downloaded</span>
              </template>
              <template slot="item" slot-scope="data">
                <span>{{ data.item.version }}</span>
                <span v-if="!data.item.stable">| Pre-release </span>
                <span v-if="data.item.downloaded">| downloaded</span>
                <span v-if="!data.item.downloaded">| not downloaded</span>
              </template>
            </v-select>

            <v-btn
                    color="success"
                    class="mr-4"
                    @click="downloadFirmware"
                    :disabled="!selectedFirmware"
            >
              Download firmware
            </v-btn>

            <v-btn
                    color="success"
                    class="mr-4 float-right"
                    @click="flashNode(selectedPort, selectedFirmware)"
                    :disabled="!canFlash"
                    :loading="flashing"
            >
              Flash Node
            </v-btn>
          </v-form>
        </v-col>
      </v-layout>
    </v-container>
  </v-main>
</template>
<script lang="ts">
  import Vue from 'vue';
  import { ipcRenderer } from 'electron'

  export default Vue.extend({
    name: 'Flash',

    data: function () {
      return {
        ports: [],
        selectedPort: null,
        firmwares: [],
        selectedFirmware: null,
        flashing: false
      }
    },
    computed: {
      canFlash: function () {
        if (this.selectedPort == null) {
          return false;
        }
        if (this.selectedFirmware == null) {
          return false;
        }
        if (this.flashing) {
          return false;
        }
        if ('downloaded' in this.selectedFirmware && this.selectedFirmware.downloaded) {
            return true;
        }
        return false;
      }
    },
    mounted() {
      ipcRenderer.on('detected-serial-devices', (event, ports) => {
        this.ports = ports
        if (!this.selectedPort && ports.length > 0) {
          this.selectedPort = ports[0];
        }
      })
      ipcRenderer.on('downloaded-firmwares', (event, files) => {
        files.forEach((file) => {
          this.firmwares.forEach((firmware) => {
            if (firmware.version === file) {
              firmware.downloaded = true;
            }
          })
        })
      })
      this.refreshPorts();
      this.fetchReleases();
    },
    methods: {
      refreshPorts: function () {
        ipcRenderer.send('get-serial-devices');
      },
      fetchReleases() {
        // https://github.com/noopkat/avrgirl-arduino
        this.firmwares = [];
        window.fetch('https://api.github.com/repos/ruudboon/tally-blaster/releases')
          .then(res => res.json())
          .then(body => {
            const releases= [];
            body.forEach((row) => {
                if (!row.draft) {
                  const release = {
                    version: row.tag_name,
                    stable: !row.prerelease,
                    binary: '',
                    downloaded: false,
                    downloadUrl: ''
                  };
                  row.assets.forEach((asset) => {
                    const ext = asset.name.substr(asset.name.lastIndexOf('.') + 1);
                    if ('bin' === ext){
                      release.downloadUrl = asset.browser_download_url;
                      release.binary = asset.name;
                      releases.push(release);
                    }
                  })
                }
              })
              if (releases.length > 0) {
                this.selectedFirmware = releases[0];
              }
              this.firmwares = releases
              ipcRenderer.send('get-firmware-downloads');
          });
      },
      downloadFirmware() {
        ipcRenderer.send('download-firmware', this.selectedFirmware);
      },
      flashNode(port, firmware) {
        ipcRenderer.send('flash-node', port, firmware);
      }
    }
  });
</script>

