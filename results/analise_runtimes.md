# Analise dos runtimes do modelo

## Objetivo

Esta analise compara os resultados obtidos com diferentes limites de tempo de execucao do modelo (`maximum_running_time`) para verificar se aumentar o tempo disponivel impactou a qualidade das solucoes.

Foram comparados os resultados disponiveis para:

- `60s`
- `300s`
- `600s`

Observacao: na verificacao anterior nao foi localizado arquivo de resultado para `30s`; por isso, a comparacao foi feita com os runtimes efetivamente encontrados.

## Resumo dos resultados

| Instancia | FO 60s | FO 300s | FO 600s | Impacto observado |
|---|---:|---:|---:|---|
| Adelaide | 151,851 | 145,951 | 145,951 | Melhorou de 60s para 300s |
| Bridgend | 100299,461 | 100299,461 | 100299,461 | Sem alteracao |
| Brisbane | 319,717 | 319,400 | 319,400 | Melhorou levemente |
| Canberra | 286,990 | 286,990 | 286,990 | Sem alteracao |
| Cardiff | 500075,860 | 500075,860 | 500075,860 | Sem alteracao |
| Edinburgh-1 | 1500155,291 | 1500147,262 | 1500147,262 | Melhorou levemente |
| Edinburgh-2 | 600045,768 | 45,349 | 45,349 | Melhorou muito |
| MiltonKeynes | 105,186 | 105,186 | 105,186 | Sem alteracao |
| Porthcawl | 25,483 | 25,483 | 25,483 | Sem alteracao |
| Suffolk | 158,202 | 158,202 | 158,202 | Sem alteracao |

## Comparacao agregada

| Runtime | Soma das funcoes objetivo | Media das funcoes objetivo |
|---|---:|---:|
| 60s | 2701623,809 | 270162,381 |
| 300s | 2101609,144 | 210160,914 |
| 600s | 2101609,144 | 210160,914 |

Ao passar de `60s` para `300s`, houve reducao total de aproximadamente `600014,665` na soma das funcoes objetivo. Isso indica melhora relevante na qualidade geral das solucoes.

Ao passar de `300s` para `600s`, a soma das funcoes objetivo permaneceu exatamente igual. Portanto, neste conjunto de execucoes, dobrar o limite de tempo de `300s` para `600s` nao gerou melhoria adicional.

## Principais observacoes

O maior impacto ocorreu na instancia `Edinburgh-2`.

Com `60s`, a solucao dessa instancia apresentou:

- Funcao objetivo: `600045,768`
- Distancia total: `45,768`
- Penalidade total: `600000,000`

Com `300s` e `600s`, a mesma instancia apresentou:

- Funcao objetivo: `45,349`
- Distancia total: `45,349`
- Penalidade total: `0,000`

Isso mostra que o limite de `60s` nao foi suficiente para o algoritmo encontrar uma solucao sem penalidade para `Edinburgh-2`. Ja com `300s`, o modelo conseguiu eliminar a penalidade.

Tambem houve melhora em `Adelaide`, `Brisbane` e `Edinburgh-1`, embora em escala menor. As demais instancias mantiveram exatamente o mesmo valor de funcao objetivo entre os tres runtimes analisados.

## Conclusao

O limitador de tempo impactou os resultados quando o runtime foi aumentado de `60s` para `300s`. O efeito mais importante foi permitir que o algoritmo encontrasse uma solucao viavel para `Edinburgh-2`, eliminando uma penalidade de `600000,000`.

Por outro lado, aumentar o limite de `300s` para `600s` nao trouxe ganho de qualidade nas solucoes observadas. Todas as funcoes objetivo permaneceram iguais entre esses dois runtimes.

Assim, para este conjunto de instancias e execucoes, `300s` parece ser o melhor compromisso entre tempo computacional e qualidade da solucao. O runtime de `60s` pode ser insuficiente para algumas instancias, enquanto `600s` nao apresentou beneficio adicional em relacao a `300s`.
