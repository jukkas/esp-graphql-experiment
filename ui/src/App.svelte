<script>
  import client from "./graphql-client";
  import gql from "graphql-tag";

  async function updateLight(device) {
    const mutation = gql`
      mutation UpdateLight($serial:String!, $light:Boolean!) {
        update_devices(where: {serial: {_eq: $serial}}, _set: {light: $light}) {
          affected_rows
        }
      }
    `;
    client.mutate({
      mutation,
      variables: {
        serial: device.serial,
        light: device.light,
      }
    });
  }

  async function updateOnline(device) {
    const mutation = gql`
      mutation UpdateOnline($serial:String!, $online:Boolean!) {
        update_devices(where: {serial: {_eq: $serial}}, _set: {online: $online}) {
          affected_rows
        }
      }
    `;
    client.mutate({
      mutation,
      variables: {
        serial: device.serial,
        online: device.online,
      }
    });
  }

  function toggleLight(device) {
    device.light = !device.light;
    updateLight(device);
  }

  function toggleOnline(device) {
    device.online = !device.online;
    updateOnline(device);
  }


const query = gql`
  subscription DeviceSubscription {
    devices(order_by: {serial: asc}) {
      serial
      name
      online
      light
      temperature
      created_at
      updated_at
    }
  }  
  `;

  const devices = client.subscribe({
    query,
});
</script>

<main>
{#if $devices && $devices.data}
  {#each $devices.data.devices as d}
    <section>
      <div>
        <div on:click={() => toggleOnline(d)}
              class="serial {d.online ? 'on':'off'}">
            {d.serial}
        </div>
        <div class="name">{d.name}</div>
      </div>
      <div class="light {d.light ? 'on':'off'}"
            on:click={() => toggleLight(d)}>
        {d.light ? 'On':'Off'}
      </div>
      <div>
        <table>
          <tr><td>Temperature</td><td>{d.temperature ? d.temperature : 'n/a'} </td></tr>
          <tr><td>Created</td><td>{d.created_at.substring(0,19)}</td></tr>
          <tr><td>Updated</td><td>{d.updated_at.substring(0,19)}</td></tr>
        </table>
      </div>
    </section>
  {/each}
{:else}
  <div>Loading data...</div>
{/if}
</main>

<style>
.serial {
  text-align: center;
}
.light {
  width: 5em;
  text-align: center;
}
.on {
  background-color: lawngreen;
}
.off {
  background-color: rgb(238, 230, 230);
}
section {
  display: flex;
  width: min-content;
  max-width: 90%;
  border: 1px solid #ccc;
  padding: 1em;
  margin: 1em auto;
  border-radius: 0.5em;
}
section > div {
  padding: 1em;
  margin: 0.5em;
}
</style>