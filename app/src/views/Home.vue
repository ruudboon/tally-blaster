<template>
    <v-main>
      <v-toolbar flat color="primary">
        <v-subheader dark>Tally host</v-subheader>
        <v-flex x1 class="pt-5">
          <v-text-field v-model="host" dark flat></v-text-field>
        </v-flex>
        <v-btn @click="setAll(host)" icon tile color="white"><v-icon>mdi-transfer-down</v-icon></v-btn>
        <v-btn @click="saveAll()" icon tile color="white"><v-icon>mdi-content-save</v-icon></v-btn>
      </v-toolbar>
      <v-container fill-height v-if="tallys.length == 0">
        <v-layout row justify-center align-center>
          <v-progress-circular
                  width="3"
                  color="green"
                  indeterminate
          ></v-progress-circular>
          <v-subheader>Listening for screaming Tally Blaster nodes on local network</v-subheader>
        </v-layout>
      </v-container>
      <v-col
              v-for="(tally, i) in tallys"
              :key="i"
              cols="12"
      >
        <v-card
                dark
        >
          <v-container>
            <v-row no-gutters>
              <v-col cols="12" sm="8">
                <v-list-item two-line>
                  <v-list-item-content>
                    <v-list-item-title class="headline">{{ tally.name }} ({{ tally.version }})</v-list-item-title>
                    <v-list-item-subtitle>{{ getTitle(tally)}}</v-list-item-subtitle>
                  </v-list-item-content>
                </v-list-item>
              </v-col>
              <v-col cols="12" sm="4">
                <v-btn
                        :color="getColorLed(tally.cameraLedEnabled, tally.ledState)"
                        class="ma-2 white--text"
                        fab
                        :disabled="tally.connectionState != 'CONNECTED'"
                        @click="toggleLed('cameraLedEnabled', tally)"
                >
                  <v-icon dark>mdi-headset</v-icon>
                </v-btn>
                <v-btn
                        :color="getColorLed(tally.viewerLedEnabled, tally.ledState)"
                        class="ma-2 white--text"
                        fab
                        :disabled="tally.connectionState != 'CONNECTED'"
                        @click="toggleLed('viewerLedEnabled', tally)"
                >
                  <v-icon dark>mdi-camera-front-variant</v-icon>
                </v-btn>
                <v-progress-circular
                        indeterminate
                        class="ma-4"
                        v-show="tally.connectionState == 'CONNECTING'"
                        color="primary"
                ></v-progress-circular>
              </v-col>
            </v-row>
            <v-row>
              <v-col cols="8" sm="6">
                <v-text-field
                        v-model="tally.sourceIp"
                        label="Source IP"
                        :disabled="tally.connectionState != 'CONNECTED'"
                        @change="sourceIpChanged(tally)"
                />
              </v-col>
              <v-col cols="4" sm="2">
                <v-text-field
                        v-model.number="tally.port"
                        label="Host Port"
                        :disabled="tally.connectionState != 'CONNECTED'"
                        @change="portChanged(tally)"
                />
              </v-col>
              <v-col cols="12" sm="2">
                <v-text-field
                        v-model.number="tally.tallyNumber"
                        label="Tally ID"
                        append-outer-icon="add"
                        @click:append-outer="tallyUp(tally)"
                        prepend-icon="remove"
                        :disabled="tally.connectionState != 'CONNECTED'"
                        @click:prepend="tallyDown(tally)"
                        @change="tallyNumberChanged(tally)"
                />
              </v-col>
              <v-col cols="12" sm="2">
                <v-select
                    v-model="tally.sourceType"
                    :items="sourceTypes"
                    label="Type"
                    item-value="value"
                    item-text="label"
                    required
                    no-data-text="No device detected"
                    @change="sourceTypeChanged(tally)"
                >
                </v-select>
              </v-col>
            </v-row>
            <v-row>
              <v-col cols="12" sm="12">
                <v-slider
                        append-icon="brightness_5"
                        prepend-icon="brightness_2"
                        min="0"
                        max="255"
                        :disabled="tally.connectionState != 'CONNECTED'"
                        :value="tally.brightness"
                        @change="changeBrightness($event, tally)"
                ></v-slider>
              </v-col>
            </v-row>
          </v-container>
          <v-bottom-navigation>
            <v-btn value="locate" :disabled="tally.connectionState != 'CONNECTED'" @click="locate(tally)">
              <span>Locate</span>
              <v-icon :class="getClassBell(tally.ledState)">mdi-bell</v-icon>
            </v-btn>

            <v-btn value="reboot" :disabled="tally.connectionState != 'CONNECTED'" @click="reboot(tally)">
              <span>Reboot</span>
              <v-icon>mdi-restart</v-icon>
            </v-btn>

            <v-btn value="save" :disabled="tally.connectionState != 'CONNECTED'" @click="save(tally)">
              <span>Save</span>
              <v-icon>mdi-content-save</v-icon>
            </v-btn>
          </v-bottom-navigation>
        </v-card>
      </v-col>
    </v-main>
</template>

<script lang="ts">
  import Vue from 'vue';
  import { mapState } from 'vuex'
  import { ipcRenderer } from 'electron'

  export default Vue.extend({
    name: 'Home',

    data: function () {
      return {
        connections: {},
        host: '192.168.0.1',
        sourceTypes: [
          {
            value: 0,
            label: 'vMix'
          },
          {
            value: 1,
            label: 'Blackmagic Atem'
          }
        ],
      }
    },
    computed: {
      ...mapState(['tallys']),
    },
    watch: {
      tallys: function (tallys) {
        tallys.forEach(tally => {
          if(this.connections[tally.name] === undefined){
            this.connections[tally.name] = "test"
          }
        })
      }
    },
    mounted() {
      ipcRenderer.on('tally-nodes', (event, tallys) => {
        this.$store.dispatch('addTallys', tallys);
      });
      this.refreshTallys();
    },
    methods: {
      refreshTallys: function () {
        ipcRenderer.send('get-tallys');
      },
      getTitle(tally) {
        return tally.host + ", " + tally.address;
      },
      getClassBell(ledState) {
        if (ledState == 3) {
          return "bell"
        }
        return "";
      },
      locate(tally) {
        tally.connection.send("locate:10");
      },
      tallyUp(tally) {
        tally.tallyNumber++;
        this.tallyNumberChanged(tally);
      },
      tallyDown(tally) {
        tally.tallyNumber--;
        this.tallyNumberChanged(tally);
      },
      tallyNumberChanged(tally) {
        const data = "tallyNumber:" + tally.tallyNumber;
        tally.connection.send(data);
      },
      sourceIpChanged(tally) {
        const data = "sourceIp:" + tally.sourceIp;
        tally.connection.send(data);
      },
      sourceTypeChanged(tally) {
        const data = "sourceType:" + tally.sourceType;
        tally.connection.send(data);
      },
      portChanged(tally) {
        const data = "port:" + tally.port;
        tally.connection.send(data);
      },
      getColor(status) {
        switch (status) {
          case 0 : //STATUS_CONNECTED: Black is connected
            return "black";
          case 1 : //STATUS_PREVIEW: Green = Preview
            return "light-green accent-3";
          case 2 : //STATUS_PROGRAM: Red = Program
            return "red darken-1";
          case 3 : //STATUS_LOCATE: White Blinking = Identify / Call
            return "white";
          case 4 : //STATUS_CONNECTWIFI: Purple Blinkning = Connecting to wifi
            return "deep-purple darken-1";
          case 5 : //STATUS_CONNECTSOURCE: Orange Blinkning = Conecting to source
            return "orange darken-1";
        }
      },
      getColorLed(enabled, status){
        if (enabled) {
          return this.getColor(status);
        }
        return "grey darken-4";
      },
      changeBrightness(value, tally) {
        const data = "brightness:" + value;
        tally.connection.send(data);
      },
      reboot(tally) {
        tally.connection.send("reboot");
      },
      save(tally) {
        tally.connection.send("save");
      },
      toggleLed(type, tally) {
        let data = type + ":";
        if (tally[type]) {
          data += "false";
        } else {
          data += "true";
        }
        tally.connection.send(data);
      },
      setAll(ip) {
        const data = "sourceIp:" + ip;
        this.tallys.forEach( tally => {
          tally.connection.send(data);
        })
      },
      saveAll() {
        this.tallys.forEach( tally => {
          tally.connection.send("save");
        })
      }
    }
  });
</script>
<style>
  .bell{
    -webkit-animation: ring 4s .7s ease-in-out infinite;
    -webkit-transform-origin: 50% 4px;
    -moz-animation: ring 4s .7s ease-in-out infinite;
    -moz-transform-origin: 50% 4px;
    animation: ring 4s .7s ease-in-out infinite;
    transform-origin: 50% 4px;
  }

  @-webkit-keyframes ring {
    0% { -webkit-transform: rotateZ(0); }
    1% { -webkit-transform: rotateZ(30deg); }
    3% { -webkit-transform: rotateZ(-28deg); }
    5% { -webkit-transform: rotateZ(34deg); }
    7% { -webkit-transform: rotateZ(-32deg); }
    9% { -webkit-transform: rotateZ(30deg); }
    11% { -webkit-transform: rotateZ(-28deg); }
    13% { -webkit-transform: rotateZ(26deg); }
    15% { -webkit-transform: rotateZ(-24deg); }
    17% { -webkit-transform: rotateZ(22deg); }
    19% { -webkit-transform: rotateZ(-20deg); }
    21% { -webkit-transform: rotateZ(18deg); }
    23% { -webkit-transform: rotateZ(-16deg); }
    25% { -webkit-transform: rotateZ(14deg); }
    27% { -webkit-transform: rotateZ(-12deg); }
    29% { -webkit-transform: rotateZ(10deg); }
    31% { -webkit-transform: rotateZ(-8deg); }
    33% { -webkit-transform: rotateZ(6deg); }
    35% { -webkit-transform: rotateZ(-4deg); }
    37% { -webkit-transform: rotateZ(2deg); }
    39% { -webkit-transform: rotateZ(-1deg); }
    41% { -webkit-transform: rotateZ(1deg); }

    43% { -webkit-transform: rotateZ(0); }
    100% { -webkit-transform: rotateZ(0); }
  }

  @-moz-keyframes ring {
    0% { -moz-transform: rotate(0); }
    1% { -moz-transform: rotate(30deg); }
    3% { -moz-transform: rotate(-28deg); }
    5% { -moz-transform: rotate(34deg); }
    7% { -moz-transform: rotate(-32deg); }
    9% { -moz-transform: rotate(30deg); }
    11% { -moz-transform: rotate(-28deg); }
    13% { -moz-transform: rotate(26deg); }
    15% { -moz-transform: rotate(-24deg); }
    17% { -moz-transform: rotate(22deg); }
    19% { -moz-transform: rotate(-20deg); }
    21% { -moz-transform: rotate(18deg); }
    23% { -moz-transform: rotate(-16deg); }
    25% { -moz-transform: rotate(14deg); }
    27% { -moz-transform: rotate(-12deg); }
    29% { -moz-transform: rotate(10deg); }
    31% { -moz-transform: rotate(-8deg); }
    33% { -moz-transform: rotate(6deg); }
    35% { -moz-transform: rotate(-4deg); }
    37% { -moz-transform: rotate(2deg); }
    39% { -moz-transform: rotate(-1deg); }
    41% { -moz-transform: rotate(1deg); }

    43% { -moz-transform: rotate(0); }
    100% { -moz-transform: rotate(0); }
  }

  @keyframes ring {
    0% { transform: rotate(0); }
    1% { transform: rotate(30deg); }
    3% { transform: rotate(-28deg); }
    5% { transform: rotate(34deg); }
    7% { transform: rotate(-32deg); }
    9% { transform: rotate(30deg); }
    11% { transform: rotate(-28deg); }
    13% { transform: rotate(26deg); }
    15% { transform: rotate(-24deg); }
    17% { transform: rotate(22deg); }
    19% { transform: rotate(-20deg); }
    21% { transform: rotate(18deg); }
    23% { transform: rotate(-16deg); }
    25% { transform: rotate(14deg); }
    27% { transform: rotate(-12deg); }
    29% { transform: rotate(10deg); }
    31% { transform: rotate(-8deg); }
    33% { transform: rotate(6deg); }
    35% { transform: rotate(-4deg); }
    37% { transform: rotate(2deg); }
    39% { transform: rotate(-1deg); }
    41% { transform: rotate(1deg); }

    43% { transform: rotate(0); }
    100% { transform: rotate(0); }
  }
</style>
