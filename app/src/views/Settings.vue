<template>
  <v-main>
    <v-container fill-height>
      <v-layout row align-center>
       <h1>config</h1>
      </v-layout>
    </v-container>
  </v-main>
</template>
<script lang="ts">
  import Vue from 'vue';
  import { ipcRenderer } from 'electron'

  export default Vue.extend({
    name: 'Settings',

    data: function () {
      return {
        settings: {},
      }
    },
    mounted() {
      ipcRenderer.on('settings-changed', (event, settings) => {
        this.settings = settings
      })
      this.getSettings();
    },
    methods: {
      getSettings: function () {
        ipcRenderer.send('get-settings');
      },
    }
  });
</script>

